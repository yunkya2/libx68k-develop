/*
 * 常駐部と非常駐部を分離したキャラクタデバイスの非常駐側サンプル。
 * CONFIG.SYSと直接実行時の引数解析、およびデバイスの登録・解除を担当する。
 */

#include <string.h>
#include <x68k/dos_inline.h>
#include <x68k/tsr.h>

#define DRIVER_NAME "MINI3   "
#define MESSAGE_SIZE 128
#define DEFAULT_MESSAGE "Hello X68000 libtsr\r\n"

/*
 * これらの変数の実体は常駐側オブジェクトにある。非常駐側から設定した値も
 * tsr_keepdev後に常駐領域へ残る。
 */
extern int minidrv3_quiet;
extern char minidrv3_message[MESSAGE_SIZE];
extern size_t minidrv3_message_length;
extern size_t minidrv3_read_position;

/*
 * CONFIG.SYSのINIT要求からも呼ばれるが、登録完了後には不要になる処理なので
 * 非常駐側へ置く。常駐側からの参照はm68k-xelf-tsrgenの
 * `-u minidrv3_configure` で許可し、最終リンクでこの定義へ解決する。
 * allow_removeが0のときは-rを受け付けない。
 */
int minidrv3_configure(int argc, char **argv, int allow_remove,
                       int *remove_requested)
{
    const char *text = NULL;

    for (int i = 1; i < argc; i++) {
        if (strcmp(argv[i], "-q") == 0) {
            minidrv3_quiet = 1;
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
    if (strlen(text) >= MESSAGE_SIZE) {
        return -1;
    }
    /* 起動時のargvではなく、常駐側オブジェクトの配列へ文字列を複製する。 */
    strcpy(minidrv3_message, text);
    minidrv3_message_length = strlen(minidrv3_message);
    minidrv3_read_position = 0;
    return 0;
}

int main(int argc, char **argv)
{
    /*
        * このファイルはm68k-xelf-tsrgenの入力に含めないため、main、configure、
        * 登録解除処理は常駐領域に残らない。
     */
    int remove_requested = 0;

    if (minidrv3_configure(argc, argv, 1, &remove_requested) < 0) {
        _dos_print("usage: minidrv3 [-q] [text] | -r\r\n");
        return 1;
    }

    /* デバイスチェインを8バイト名で検索し、未登録ならNULLを得る。 */
    tsr_device_t dev = tsr_finddev(DRIVER_NAME);
    if (remove_requested) {
        if (dev == NULL) {
            _dos_print("MINI3 is not installed\r\n");
            return 1;
        }
        /* 登録を外して、tsr_keepdevで常駐させたメモリブロックを解放する。 */
        if (tsr_freedev(dev) < 0) {
            _dos_print("MINI3 cannot be removed\r\n");
            return 1;
        }
        return 0;
    }

    if (dev != NULL) {
        _dos_print("MINI3 is already installed\r\n");
        return 0;
    }
    if (!minidrv3_quiet) {
        _dos_print("MINI3 installed\r\n");
    }
    /*
     * デバイスを登録して常駐する。分離型ではNULLを指定すると
     * m68k-xelf-tsrgenが生成した常駐領域の末尾までが保持される。
     */
    tsr_keepdev(NULL, 0);
}