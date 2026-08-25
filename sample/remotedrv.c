/*
 * 空の読み取り専用リモートドライブを接続するlibtsrサンプル。
 * リモート要求への応答、複数ユニットの接続、常駐データの再取得と解除を示す。
 */

#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <x68k/dos_inline.h>
#include <x68k/tsr.h>

#define DRIVER_NAME "\x01REMOTDRV"
#define MAX_UNITS 16
#define DATA_MAGIC 0x52445256UL

/*
 * _DEV_REMOTEを加えると、DOSのリモートドライブ要求を受けるデバイスになる。
 * 名前の先頭バイトはブロックデバイスのユニット数の初期値である。
 */
TSR_DEVICE(_DEV_BDEV | _DEV_REMOTE, DRIVER_NAME, remotedrv_handler);

/* ハンドラが常駐後にも使えるヒープを確保し、不要な環境は残さない。 */
TSR_HEAP(1024);
TSR_NO_ENVIRON();

/* dの役割はlocaldrvと同じで、再実行時には常駐側のdataへ差し替える。 */
static struct dos_dpb_remote *dpb_table[MAX_UNITS];
static struct data {
    uint32_t magic;
    int units;
    struct dos_dpb_remote dpbs[MAX_UNITS];
} data = {
    .magic = DATA_MAGIC,
    .units = 1,
};
static struct data *d = &data;
static char print_buf[80];

static int is_root_directory(const struct dos_namestbuf *name)
{
    return name->path[0] == '\t' && name->path[1] == '\0';
}

/*
 * ファイルが存在しない、読み取り専用のリモートドライブ。
 * リモートドライブではファイル操作ごとの要求(0x41以降)を実装する。
 */
int remotedrv_handler(struct dos_req_header *rh)
{
    if ((rh->command & 0x7f) != 0x40 && rh->unit >= d->units) {
        return 0x7008;         // 不正なユニット番号
    }

    switch (rh->command & 0x7f) {
    case 0x40:                 // INIT
        /* 常駐領域の終端とユニット数を返し、再実行用データも保存する。 */
        rh->addr = tsr_getresidentend();
        rh->attr = d->units;   // ユニット数
        tsr_setdevdata(TSR_THISDEV, d);
        return 0;

    case 0x41:                 // ディレクトリ検索
        rh->status = is_root_directory(rh->addr) ? 0 : _DOSE_NODIR;
        return 0;

    case 0x42:                 // ディレクトリ作成
    case 0x43:                 // ディレクトリ削除
    case 0x44:                 // ファイル名変更
    case 0x45:                 // ファイル削除
        rh->status = _DOSE_RDONLY;
        return 0;

    case 0x46:                 // ファイル属性取得/設定
        rh->status = rh->attr == 0xff ? _DOSE_NOENT : _DOSE_RDONLY;
        return 0;

    case 0x47:                 // FILES
    case 0x48: {               // NFILES
        /* 空のドライブなので、最初の検索で列挙終了を返す。 */
        struct dos_filbuf *files = (struct dos_filbuf *)(uintptr_t)rh->status;
        files->dirpos = 0xffff;
        rh->status = _DOSE_NOMORE;
        return 0;
    }

    case 0x49:                 // ファイル作成
        rh->status = _DOSE_RDONLY;
        return 0;

    case 0x4a:                 // ファイルオープン
        rh->status = _DOSE_NOENT;
        return 0;

    case 0x4b:                 // ファイルクローズ
    case 0x4c:                 // ファイル読み込み
    case 0x4e:                 // ファイルシーク
    case 0x4f:                 // ファイル更新時刻取得/設定
        rh->status = _DOSE_BADF;
        return 0;

    case 0x4d:                 // ファイル書き込み
        rh->status = _DOSE_RDONLY;
        return 0;

    case 0x50: {               // 容量取得
        /* 実体を持たないため、最小構成かつ空き容量0として応答する。 */
        uint16_t *capacity = rh->addr;
        capacity[0] = 0;       // 空きクラスタ数
        capacity[1] = 1;       // 総クラスタ数
        capacity[2] = 1;       // 1クラスタ当たりのセクタ数
        capacity[3] = 512;     // 1セクタ当たりのバイト数
        rh->status = 0;        // 空き容量
        return 0;
    }

    case 0x51:                 // DRVCTRL
        rh->attr = 0x0a;       // 挿入済み、書き込み禁止、レディ
        rh->status = 0;
        return 0;

    case 0x52: {               // DPB取得
        uint8_t *dpb = rh->addr;
        memset(dpb, 0, 16);
        *(uint16_t *)&dpb[0] = 512;
        dpb[2] = 0;            // 1クラスタ当たり1セクタ
        rh->status = 0;
        return 0;
    }

    case 0x53:                 // ディスク読み込み
        rh->status = 0;
        return 0;

    case 0x54:                 // ディスク書き込み
        rh->status = _DOSE_RDONLY;
        return 0;

    case 0x55:                 // IOCTRL
        rh->status = _DOSE_CANTIOC;
        return 0;

    case 0x56:                 // アボート
    case 0x57:                 // メディア交換検査
    case 0x58:                 // 排他制御
        rh->status = 0;
        return 0;

    default:
        return 0x7003;         // 不正なコマンド
    }
}

static void show_status(void)
{
    for (int unit = 0; unit < d->units; unit++) {
        /* 接続後に割り当てられた論理ドライブ名を取得する。 */
        int drive = tsr_getlogicaldrive(d->dpbs[unit].drive);
        sprintf(print_buf, "%c: remote drive (unit %d)\r\n",
                drive >= 0 ? 'A' + drive : '?', unit);
        _dos_print(print_buf);
    }
}

static int parse_units(const char *arg)
{
    char *end;
    long units = strtol(arg, &end, 0);

    if (*arg == '\0' || *end != '\0' || units < 1 || units > MAX_UNITS) {
        return -1;
    }
    return units;
}

int main(int argc, char **argv)
{
    /* 同名デバイスの有無で、登録処理か再実行時の操作かを判別する。 */
    tsr_device_t dev = tsr_finddev(DRIVER_NAME);
    int remove_requested = argc == 2 && strcmp(argv[1], "-r") == 0;

    if (dev == NULL) {
        if (remove_requested) {
            _dos_print("常駐していません\r\n");
            _dos_exit2(1);
        }
        if (argc > 2 || (argc == 2 && (d->units = parse_units(argv[1])) < 0)) {
            _dos_print("usage: remotedrv [units] | -r\r\n");
            _dos_exit2(1);
        }

        for (int i = 0; i < d->units; i++) {
            /* リモートDPBはセクタ単位のローカルI/Oを使用しない。 */
            d->dpbs[i].sectbytes = 0;
            dpb_table[i] = &d->dpbs[i];
        }
        /* 空いている論理ドライブへ、指定数のリモートユニットを接続する。 */
        if (tsr_attachdrive(-1, d->units, (struct dos_dpb **)dpb_table,
                            TSR_THISDEV) < 0) {
            _dos_print("リモートドライブを追加できません\r\n");
            _dos_exit2(1);
        }
        /* 再実行時に常駐側のDPBやユニット数を参照できるようにする。 */
        tsr_setdevdata(TSR_THISDEV, d);
        /* デバイスを常駐させ、コマンドの初回実行を終了する。 */
        tsr_keepdev(NULL, 0);
    } else {
        /* ここで取得するのは再実行側ではなく、常駐側にあるdataである。 */
        d = tsr_getdevdata(dev);
        if (d == NULL || d->magic != DATA_MAGIC ||
            d->units < 1 || d->units > MAX_UNITS) {
            _dos_print("常駐情報に互換性がありません\r\n");
            _dos_exit2(1);
        }
        if (!remove_requested) {
            if (argc != 1) {
                _dos_print("usage: remotedrv [units] | -r\r\n");
                _dos_exit2(1);
            }
            show_status();
            return 0;
        }
        /* 解放可能性を確認してから、ドライブ、常駐領域の順で解除する。 */
        if (!tsr_isfreeable(dev)) {
            _dos_print("常駐解除できません\r\n");
            _dos_exit2(1);
        }
        if (tsr_detachdrive(dev) < 0) {
            _dos_print("ドライブが使用中です\r\n");
            _dos_exit2(1);
        }
        if (tsr_freedev(dev) < 0) {
            _dos_print("常駐解除できません\r\n");
            _dos_exit2(1);
        }
    }
    return 0;
}
