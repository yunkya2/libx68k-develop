/*
 * 1つの実行ファイルで複数のキャラクタデバイスを登録・解除するサンプル。
 * MDEV0、MDEV1、MDEV2は、それぞれ異なる文字列をREADで返す。
 */

#include <string.h>
#include <x68k/dos_inline.h>
#include <x68k/tsr.h>

#define FIRST_DEVICE_NAME "MDEV0   "

int mdev_handler(struct dos_req_header *rh, tsr_device_t dev);

/*
 * FIRST、NEXT、TAILは連続したデバイスヘッダ列を定義する。識別子1、2は
 * マクロが生成するシンボル名とデバイス間リンクに使われる。
 */
TSR_DEVICE_FIRST(_DEV_CDEV, FIRST_DEVICE_NAME, mdev_handler, 1, 2);
TSR_DEVICE_NEXT(_DEV_CDEV, "MDEV1   ", mdev_handler, 1, 2);
TSR_DEVICE_TAIL(_DEV_CDEV, "MDEV2   ", mdev_handler, 2);

TSR_HEAP(1024);
TSR_NO_ENVIRON();

static struct device_state {
    const char *message;
    size_t position;
} devices[] = {
    { "Hello from MDEV0\r\n", 0 },
    { "Hello from MDEV1\r\n", 0 },
    { "Hello from MDEV2\r\n", 0 },
};

int mdev_handler(struct dos_req_header *rh, tsr_device_t dev)
{
    struct device_state *device = tsr_getdevdata(dev);
    size_t length = strlen(device->message);

    switch (rh->command) {
    case 0x00:
        /* Human68kへ、このドライバ列が常駐に必要とする領域の末尾を返す。 */
        rh->addr = tsr_getresidentend();
        return 0;
    case 0x04: {
        size_t available = length - device->position;
        size_t count = rh->status < available ? rh->status : available;

        memcpy(rh->addr, device->message + device->position, count);
        device->position += count;
        rh->status = count;
        return 0;
    }
    case 0x05:
        /* 次の1バイトを先読みし、読み取り位置は進めない。 */
        rh->attr = device->position < length
                       ? (uint8_t)device->message[device->position] : 0;
        return 0;
    case 0x06:
        return 0;
    case 0x07:
        device->position = 0;
        return 0;
    case 0x08:
    case 0x09:
        rh->status = 0;
        return 0;
    default:
        return 0x7003;
    }
}

int main(int argc, char **argv)
{
    int remove_requested = argc == 2 && strcmp(argv[1], "-r") == 0;

    if (argc > 2 || (argc == 2 && !remove_requested)) {
        _dos_print("usage: multidrv [-r]\r\n");
        return 1;
    }

    /* 複数デバイス列の検索と解除には、必ず先頭デバイスを使用する。 */
    tsr_device_t first = tsr_finddev(FIRST_DEVICE_NAME);
    if (remove_requested) {
        if (first == NULL) {
            _dos_print("MDEV0-MDEV2 are not installed\r\n");
            return 1;
        }
        /* 先頭ヘッダを渡すと、FIRSTからTAILまでがまとめて解除される。 */
        if (tsr_freedev(first) < 0) {
            _dos_print("MDEV0-MDEV2 cannot be removed\r\n");
            return 1;
        }
        return 0;
    }

    if (first != NULL) {
        _dos_print("MDEV0-MDEV2 are already installed\r\n");
        return 0;
    }

    _dos_print("MDEV0-MDEV2 installed\r\n");
    /* 共通ハンドラがデバイスヘッダから固有の状態を取得できるようにする。 */
    tsr_setdevdata(TSR_THISDEV, &devices[0]);
    tsr_setdevdata(TSR_THISDEV_ID(1), &devices[1]);
    tsr_setdevdata(TSR_THISDEV_ID(2), &devices[2]);
    /* TSR_DEVICE_FIRSTからTAILまでのヘッダ列を接続して常駐する。 */
    tsr_keepdev(NULL, 0);
}