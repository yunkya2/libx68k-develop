/*
 * 常駐部と非常駐部を分離したキャラクタデバイスの常駐側サンプル。
 * デバイスヘッダ、READで返す文字列、Human68kの要求ハンドラを保持する。
 */

#include <string.h>
#include <x68k/dos_inline.h>
#include <x68k/tsr.h>

#define DRIVER_NAME "MINI3   "
#define MESSAGE_SIZE 128

/*
 * デバイスヘッダ、strategy/interrupt入口、Cハンドラを常駐側へ置く。
 * このソースから作るオブジェクトだけをm68k-xelf-tsrgenへ渡す。
 */
TSR_DEVICE(_DEV_CDEV, DRIVER_NAME, minidrv3_handler);

/* 固定ヒープも常駐側へ置き、常駐後のライブラリ処理から利用可能にする。 */
TSR_HEAP(1024);
TSR_NO_ENVIRON();

/* mainが設定し、常駐後もハンドラから参照できるドライバ固有データ。 */
int minidrv3_quiet;
char minidrv3_message[MESSAGE_SIZE];
size_t minidrv3_message_length;
size_t minidrv3_read_position;

/*
 * INIT時だけ呼び出す非常駐側の引数設定処理。登録完了後は非常駐領域が
 * 解放されるため、この関数を再び呼び出してはならない。
 */
extern int minidrv3_configure(int argc, char **argv, int allow_remove,
                              int *remove_requested);

int minidrv3_handler(struct dos_req_header *rh)
{
    switch (rh->command) {
    case 0x00: {
        char *argv[8];
        int remove_requested = 0;

        /* CONFIG.SYSからの登録でも、直接実行時と同じ-qを解釈する。 */
        int argc = tsr_parseinitparam((void *)rh->status, argv, 8);

        /* 分離型なので、m68k-xelf-tsrgenが定めた常駐部の末尾を返す。 */
        rh->addr = tsr_getresidentend();
        if (minidrv3_configure(argc, argv, 0, &remove_requested) < 0) {
            _dos_print("MINI3: usage: [-q] [text]\r\n");
            return 0x7003;
        }
        if (!minidrv3_quiet) {
            _dos_print("MINI3 installed\r\n");
        }
        return 0;
    }
    case 0x04: {
        /* 未読部分から要求サイズまで返し、statusへ実バイト数を設定する。 */
        size_t available = minidrv3_message_length - minidrv3_read_position;
        size_t count = rh->status < available ? rh->status : available;

        memcpy(rh->addr, minidrv3_message + minidrv3_read_position, count);
        minidrv3_read_position += count;
        rh->status = count;
        return 0;
    }
    case 0x08:
    case 0x09:
        /* WRITE/WRITEVERIFYではデータを受け取らない。 */
        rh->status = 0;
        return 0;
    case 0x05:
        /* 次の1バイトを先読みする。通常のREADと異なり位置は進めない。 */
        rh->attr = minidrv3_read_position < minidrv3_message_length
                       ? (uint8_t)minidrv3_message[minidrv3_read_position] : 0;
        return 0;
    case 0x06:
        /* 入力ステータス要求には、入力可能を表す成功を返す。 */
        return 0;
    case 0x07:
        /* 入力バッファクリア要求で読み取り位置を先頭へ戻す。 */
        minidrv3_read_position = 0;
        return 0;
    default:
        /* 未対応のデバイスコマンド。 */
        return 0x7003;
    }
}