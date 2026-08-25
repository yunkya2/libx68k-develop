/*
 * CONFIG.SYSと直接実行の両方で登録できるキャラクタデバイスのサンプル。
 * 登録時に指定した文字列をREADで返し、直接実行時は-rで登録解除できる。
 */

#include <string.h>
#include <x68k/dos_inline.h>
#include <x68k/tsr.h>

#define DRIVER_NAME "MINI2   "
#define MESSAGE_SIZE 128
#define DEFAULT_MESSAGE "Hello X68000 libtsr\r\n"

/* デバイスヘッダとCハンドラへ接続するstrategy/interrupt入口を生成する。 */
TSR_DEVICE(_DEV_CDEV, DRIVER_NAME, minidrv2_handler);

/* 常駐後に利用可能な固定ヒープを確保し、環境変数配列の構築を省略する。 */
TSR_HEAP(1024);
TSR_NO_ENVIRON();

static int quiet;
static char message[MESSAGE_SIZE];
static size_t message_length;
static size_t read_position;

static int configure(int argc, char **argv, int allow_remove,
                     int *remove_requested)
{
    const char *text = NULL;

    for (int i = 1; i < argc; i++) {
        if (strcmp(argv[i], "-q") == 0) {
            quiet = 1;
        } else if (allow_remove && strcmp(argv[i], "-r") == 0) {
            *remove_requested = 1;
        } else if (argv[i][0] == '-') {
            return -1;
        } else if (text == NULL) {
            text = argv[i];
        } else {
            return -1;
        }
    }

    if (*remove_requested) {
        return text == NULL ? 0 : -1;
    }
    if (text == NULL) {
        text = DEFAULT_MESSAGE;
    }
    if (strlen(text) >= sizeof(message)) {
        return -1;
    }
    /* 引数へのポインタを保持せず、常駐データとして所有する静的配列へ複製する。 */
    strcpy(message, text);
    message_length = strlen(message);
    read_position = 0;
    return 0;
}

int minidrv2_handler(struct dos_req_header *rh)
{
    switch (rh->command) {
    case 0x00: {
        char *argv[8];
        int remove_requested = 0;

        /* CONFIG.SYSのNUL区切りパラメータをargc/argv形式へ変換する。 */
        int argc = tsr_parseinitparam((void *)rh->status, argv, 8);

        /* Human68kへ、このドライバが常駐に必要とする領域の末尾を返す。 */
        rh->addr = tsr_getresidentend();
        if (configure(argc, argv, 0, &remove_requested) < 0) {
            _dos_print("MINI2: usage: [-q] [text]\r\n");
            return 0x7003;
        }
        if (!quiet) {
            _dos_print("MINI2 installed\r\n");
        }
        return 0;
    }
    case 0x04: {
        /* 未読部分から要求サイズまで返し、statusへ実バイト数を設定する。 */
        size_t available = message_length - read_position;
        size_t count = rh->status < available ? rh->status : available;

        memcpy(rh->addr, message + read_position, count);
        read_position += count;
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
        rh->attr = read_position < message_length
                       ? (uint8_t)message[read_position] : 0;
        return 0;
    case 0x06:
        /* 入力ステータス要求には、入力可能を表す成功を返す。 */
        return 0;
    case 0x07:
        /* 入力バッファクリア要求で読み取り位置を先頭へ戻す。 */
        read_position = 0;
        return 0;
    default:
        /* 未対応のデバイスコマンド。 */
        return 0x7003;
    }
}

int main(int argc, char **argv)
{
    int remove_requested = 0;

    if (configure(argc, argv, 1, &remove_requested) < 0) {
        _dos_print("usage: minidrv2 [-q] [text] | -r\r\n");
        return 1;
    }

    /*
     * tsr_finddevはHuman68kのデバイスチェインを検索し、8バイト名が一致する
     * デバイスのハンドルを返す。見つからない場合はNULLを返す。
     */
    tsr_device_t dev = tsr_finddev(DRIVER_NAME);
    if (remove_requested) {
        if (dev == NULL) {
            _dos_print("MINI2 is not installed\r\n");
            return 1;
        }
        /*
         * tsr_freedevはデバイスチェインからヘッダを外し、tsr_keepdevで
         * 常駐させたプロセスのメモリブロックを解放する。解除できなければ
         * 負値を返す。
         */
        if (tsr_freedev(dev) < 0) {
            _dos_print("MINI2 cannot be removed\r\n");
            return 1;
        }
        return 0;
    }

    if (dev != NULL) {
        _dos_print("MINI2 is already installed\r\n");
        return 0;
    }
    if (!quiet) {
        _dos_print("MINI2 installed\r\n");
    }
    /*
     * tsr_keepdevは自分のヘッダをデバイスチェインへ登録し、現在の
     * プロセスを常駐させる。終了アドレスNULLはライブラリの既定値、0は
     * Human68kへ返す終了コードであり、この関数からは戻らない。
     */
    tsr_keepdev(NULL, 0);
}