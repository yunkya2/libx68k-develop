/*
 * CONFIG.SYSからのみ登録する最小キャラクタデバイスのサンプル。
 * 登録時に指定した文字列をREADで返し、直接実行による登録・解除は行わない。
 */

#include <string.h>
#include <x68k/dos_inline.h>
#include <x68k/tsr.h>

#define DRIVER_NAME "MINI1   "
#define MESSAGE_SIZE 128
#define DEFAULT_MESSAGE "Hello X68000 libtsr\r\n"

/*
 * TSR_DEVICEはHuman68k形式のデバイスヘッダと、Cで記述したハンドラを
 * 呼び出すstrategy/interrupt入口を生成する。_DEV_CDEVはこのデバイスが
 * キャラクタデバイスであることを表す。
 */
TSR_DEVICE(_DEV_CDEV, DRIVER_NAME, minidrv1_handler);

/*
 * TSR_HEAPは常駐後も使用できる固定ヒープを静的領域内に確保する。
 * TSR_NO_ENVIRONは起動時の環境変数配列を作らず、固定ヒープの消費を抑える。
 */
TSR_HEAP(1024);
TSR_NO_ENVIRON();

static int quiet;
static char message[MESSAGE_SIZE];
static size_t message_length;
static size_t read_position;

static int configure(int argc, char **argv)
{
    const char *text = NULL;

    for (int i = 1; i < argc; i++) {
        if (argv[i][0] == '-' && argv[i][1] == 'q' && argv[i][2] == '\0') {
            quiet = 1;
        } else if (argv[i][0] == '-') {
            return -1;
        } else if (text == NULL) {
            text = argv[i];
        } else {
            return -1;
        }
    }

    if (text == NULL) {
        text = DEFAULT_MESSAGE;
    }
    if (strlen(text) >= sizeof(message)) {
        return -1;
    }
    /* INITパラメータ領域は常駐後に使えないため、常駐側の配列へ複製する。 */
    strcpy(message, text);
    message_length = strlen(message);
    read_position = 0;
    return 0;
}

int minidrv1_handler(struct dos_req_header *rh)
{
    /* rh->commandはHuman68kから要求されたデバイスコマンド番号である。 */
    switch (rh->command) {
    case 0x00: {
        char *argv[8];

        /*
         * INIT要求のstatusにはCONFIG.SYSのDEVICE行で指定されたパラメータが
         * 入る。tsr_parseinitparamはこれを通常のargc/argv形式へ変換する。
         * argvの文字列は元のパラメータ領域を指し、配列末尾はNULLになる。
         */
        int argc = tsr_parseinitparam((void *)rh->status, argv, 8);

        /*
         * INIT要求のaddrへ常駐領域の終了アドレスを返す。
         * tsr_getresidentendは通常型・非常駐部分離型の違いを吸収する。
         */
        rh->addr = tsr_getresidentend();
        if (configure(argc, argv) < 0) {
            _dos_print("MINI1: usage: [-q] [text]\r\n");
            return 0x7003;
        }
        if (!quiet) {
            _dos_print("MINI1 installed\r\n");
        }
        return 0;
    }
    case 0x04: {
        /*
         * READのstatusは要求バイト数、addrは出力先である。未読部分から
         * 要求サイズまでコピーし、statusを実際に返したバイト数へ更新する。
         */
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
        /* 先読み入力は次の1バイトをattrへ返すが、読み取り位置は進めない。 */
        rh->attr = read_position < message_length
                       ? (uint8_t)message[read_position] : 0;
        return 0;
    case 0x06:
        /* 入力ステータス要求には、入力可能を表す成功を返す。 */
        return 0;
    case 0x07:
        /* READBUFCLEARで次のREADを文字列の先頭へ戻す。 */
        read_position = 0;
        return 0;
    default:
        /* 0x7003は未対応コマンドを表すHuman68kのデバイスエラーである。 */
        return 0x7003;
    }
}

void _start(void)
{
    /* CONFIG.SYSからロードされ、Human68kがINIT要求とチェイン接続を行う。 */
}