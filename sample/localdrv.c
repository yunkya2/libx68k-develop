/*
 * 直接実行して通常のローカルドライブとしてRAMディスクを接続するlibtsrサンプル。
 * BPB/DPBの準備、複数ユニットの接続、常駐データの再取得、解除までを示す。
 */

#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include <stdint.h>
#include <x68k/iocs_inline.h>
#include <x68k/dos_inline.h>
#include <x68k/tsr.h>

#define DRIVER_NAME "\x01LOCALDRV"
#define MAX_UNITS 16
#define SECTOR_BYTES 1024
#define DATA_MAGIC 0x4c445256UL

/*
 * _DEV_BDEVを指定すると、通常のローカルブロックデバイスとして登録される。
 * ブロックデバイス名の先頭バイトはユニット数であり、ここでは初期値を1とする。
 * 直接実行時のユニット数はtsr_attachdrive()へ渡す引数で設定する。
 */
TSR_DEVICE(_DEV_BDEV, DRIVER_NAME, localdrv_handler);

/* 常駐後にもハンドラが利用するヒープを、常駐ブロック内に確保する。 */
TSR_HEAP(1024);
/* このサンプルは常駐部から環境変数を参照しないため、環境を残さない。 */
TSR_NO_ENVIRON();

/*
 * dataとそのメンバーは、デバイスハンドラから参照するため常駐領域に置く。
 * dは初回実行時には静的なdataを指し、再実行時には常駐中のdataを指す。
 */
static struct dos_dpb *dpb_table[MAX_UNITS];
static struct data {
    uint32_t magic;
    int units;
    struct dos_bpb bpbs[MAX_UNITS];
    struct dos_dpb dpbs[MAX_UNITS];
    uint8_t *ramdisks[MAX_UNITS];
} data = {
    .magic = DATA_MAGIC,
};
static struct data *d = &data;
char printf_buf[256];

//#define DEBUG
#ifdef DEBUG
#define DPRINTF(fmt, ...) do { \
    sprintf(printf_buf, fmt, ##__VA_ARGS__); \
    _iocs_b_print(printf_buf); \
} while (0)
#else
#define DPRINTF(fmt, ...) do { } while (0)
#endif

/* Human68kから届くブロックデバイス要求を処理する常駐ハンドラ。 */
int localdrv_handler(struct dos_req_header *rh)
{
    int res = 0;
    uint32_t sector, nsect;
    struct dos_bpb *bpb;

    DPRINTF("Command=0x%02x\r\n", rh->command);
    switch (rh->command) {
    case 0x00: {               // INIT
        /* localdrvは直接実行専用なので、CONFIG.SYSからの登録を拒否する。 */
        rh->addr = tsr_getresidentend();
        _dos_print("LOCALDRV: CONFIG.SYSからは登録できません\r\n");
        return 0x7003;
    }
    case 0x01:                 // MEDIACHECK
        *(int8_t *)&rh->addr = 1;   // メディア交換なし
        break;
    case 0x02:                 // BUILDBPB
        if (rh->unit >= d->units) {
            res = 0x7008;
        } else {
            rh->status = (uint32_t)&d->bpbs[rh->unit];
        }
        break;
    case 0x05:                 // DRVCTRL
        rh->attr = 0x02;        // 挿入済み、排出不可、書込可能、レディ
        break;
    case 0x04:                 // READ
    case 0x08:                 // WRITE
    case 0x09:                 // WRITEVERIFY
        sector = (uint32_t)(uintptr_t)rh->fcb;
        nsect = rh->status;
        DPRINTF("%s: unit=%d sector=%lu nsect=%lu buf=%p\r\n",
                rh->command == 0x04 ? "READ" : "WRITE",
                rh->unit, sector, nsect, rh->addr);
        if (rh->unit >= d->units) {
            res = 0x7008;
            break;
        }
        bpb = &d->bpbs[rh->unit];
        if (sector > bpb->sects || nsect > bpb->sects - sector) {
            res = 0x7008;
            break;
        }
        /* セクタ番号をRAM上の実体へ変換してデータを転送する。 */
        if (rh->command == 0x04) {
            memcpy(rh->addr, &d->ramdisks[rh->unit][sector * SECTOR_BYTES],
                   nsect * SECTOR_BYTES);
        } else {
            memcpy(&d->ramdisks[rh->unit][sector * SECTOR_BYTES], rh->addr,
                   nsect * SECTOR_BYTES);
        }
        break;
    default:
        res = 0x7003;
        break;
    }
    return res;
}

static void *prepare_ramdisk_sector(void *arg, int sect)
{
    /* tsr_formatdrive()に、指定セクタを書き込むRAM上の位置を渡す。 */
    if (sect < 0) return NULL;
    return &((uint8_t *)arg)[(uint32_t)sect * SECTOR_BYTES];
}

static int make_ramdisk(int unit, const char *arg)
{
    char *end;
    unsigned long size = strtoul(arg, &end, 0);

    // 引数の単位はKiB。1セクタも1KiBなので値は総セクタ数になる。
    if (*arg == '\0' || *end != '\0' || size < 16 || size > 65535) return -1;

    /* BPBでHuman68kに媒体の論理構造を知らせる。 */
    struct dos_bpb *bpb = &d->bpbs[unit];
    bpb->sectbytes = SECTOR_BYTES;
    bpb->sectclust = 1;
    bpb->fatnum = 2;
    bpb->resvsects = 1;
    bpb->rootent = 192;
    bpb->sects = size;
    bpb->mediabyte = 0xf9;  // RAMDISK
    unsigned long fatbytes = size < 4085 ? (size + 2) * 3 / 2
                                         : (size + 2) * 2;
    bpb->fatsects = (fatbytes + SECTOR_BYTES - 1) / SECTOR_BYTES;
    bpb->sectslong = 0;

    // 常駐ブロックとの間に使用不能な空きを残さないよう、上位アドレス側から確保する。
    d->ramdisks[unit] = _dos_malloc2(2, size * SECTOR_BYTES);
    if ((intptr_t)d->ramdisks[unit] < 0) {
        d->ramdisks[unit] = NULL;
        return -2;
    }
    return 0;
}

static void show_status(void)
{
    for (int unit = 0; unit < d->units; unit++) {
        /* DPBの物理ドライブ番号を、利用者が見る論理ドライブ番号へ変換する。 */
        int drive = tsr_getlogicaldrive(d->dpbs[unit].drive);
        sprintf(printf_buf, "%c: %u KiB\r\n",
                drive >= 0 ? 'A' + drive : '?',
                d->bpbs[unit].sects);
        _dos_print(printf_buf);
    }
}

int main(int argc, char **argv)
{
    /* 同名デバイスを探すことで、初回実行と常駐後の再実行を判別する。 */
    tsr_device_t dev = tsr_finddev(DRIVER_NAME);
    int remove_requested = argc == 2 && strcmp(argv[1], "-r") == 0;

    if (dev == NULL) {
        if (remove_requested) {
            _dos_print("常駐していません\r\n");
            _dos_exit2(1);
        }
        if (argc < 2 || argc > MAX_UNITS + 1) {
            _dos_print("usage: localdrv size_kib [size_kib ...] | -r\r\n");
            _dos_exit2(1);
        }
        d->units = argc - 1;
        for (int i = 0; i < d->units; i++) {
            int res = make_ramdisk(i, argv[i + 1]);
            if (res < 0) {
                _dos_print(res == -2 ? "メモリが足りません\r\n"
                                     : "RAMディスクサイズが不正です\r\n");
                _dos_exit2(1);
            }
            /* BPBから接続に必要なDPBを作り、空のFAT媒体を初期化する。 */
            if (tsr_bpb2dpb(&d->bpbs[i], &d->dpbs[i]) < 0 ||
                tsr_formatdrive(&d->dpbs[i], prepare_ramdisk_sector, NULL,
                                d->ramdisks[i]) < 0) {
                _dos_print("RAMディスクをフォーマットできません\r\n");
                _dos_exit2(1);
            }
            dpb_table[i] = &d->dpbs[i];
        }
        /* -1を指定すると、空いている論理ドライブへまとめて接続される。 */
        if (tsr_attachdrive(-1, d->units, dpb_table, TSR_THISDEV) < 0) {
            _dos_print("ドライブを追加できません\r\n");
            _dos_exit2(1);
        }
        /* 再実行したプログラムが常駐データを取得できるよう保存する。 */
        tsr_setdevdata(TSR_THISDEV, d);
        /* ここからデバイスと必要なメモリを常駐させ、呼び出し元へ戻る。 */
        tsr_keepdev(NULL, 0);
    } else {
        /* devは常駐側を指すため、以降は常駐側のデータだけを操作する。 */
        d = tsr_getdevdata(dev);
        if (d == NULL || d->magic != DATA_MAGIC ||
            d->units < 1 || d->units > MAX_UNITS) {
            _dos_print("常駐情報に互換性がありません\r\n");
            _dos_exit2(1);
        }
        if (!remove_requested) {
            show_status();
            return 0;
        }
        /* 使用中のハンドラなどがあれば、常駐領域を解放してはならない。 */
        if (!tsr_isfreeable(dev)) {
            _dos_print("常駐解除できません\r\n");
            _dos_exit2(1);
        }
        /* 常駐領域より先に、DOSのドライブ管理から切り離す。 */
        if (tsr_detachdrive(dev) < 0) {
            _dos_print("ドライブが使用中です\r\n");
            _dos_exit2(1);
        }
        /* RAMディスク本体は別途確保した領域なので明示的に解放する。 */
        for (int i = 0; i < d->units; i++) {
            _dos_mfree(d->ramdisks[i]);
        }
        /* 最後にデバイス本体と常駐ブロックを解放する。 */
        if (tsr_freedev(dev) < 0) {
            _dos_print("常駐解除できません\r\n");
            _dos_exit2(1);
        }
    }
    return 0;
}
