#ifndef _DOS_INTERNAL_H_
#define _DOS_INTERNAL_H_

#include <x68k/dos.h>

// Device header structure
struct dos_devheader {
    struct dos_devheader *next; // +0x00.l 次のデバイスヘッダ
    uint16_t attr;              // +0x04.w デバイス属性
    void *strategy;             // +0x06.l ストラテジルーチン
    void *interrupt;            // +0x0a.l 割り込みルーチン
    char name[8];               // +0x0e.b デバイス名
} __attribute__((__packed__, __aligned__(2)));

// Request header structure
struct dos_req_header {
    uint8_t magic;              // +0x00.b Constant
    uint8_t unit;               // +0x01.b Unit number
    uint8_t command;            // +0x02.b Command code
    uint8_t errl;               // +0x03.b Error code low
    uint8_t errh;               // +0x04.b Error code high
    uint8_t reserved[8];        // +0x05 .. +0x0c not used
    uint8_t attr;               // +0x0d.b Attribute / Seek mode
    void *addr;                 // +0x0e.l Buffer address
    uint32_t status;            // +0x12.l Bytes / Buffer / Result status
    void *fcb;                  // +0x16.l FCB
} __attribute__((__packed__, __aligned__(2)));

// BIOS Parameter Block (FAT12/FAT16)
struct dos_bpb {
    uint16_t sectbytes;         // +0x00.b 1セクタあたりのバイト数
    uint8_t sectclust;          // +0x02.b 1クラスタあたりのセクタ数
    uint8_t fatnum;             // +0x03.b FAT領域の個数 (bit7=1ならIntel FAT)
    uint16_t resvsects;         // +0x04.w 予約セクタ数 (=FAT領域の先頭セクタ番号)
    uint16_t rootent;           // +0x06.w ルートディレクトリエントリ数
    uint16_t sects;             // +0x08.w 全セクタ数 (wordに収まらない場合は0)
    uint8_t mediabyte;          // +0x0a.b メディアバイト
    uint8_t fatsects;           // +0x0b.b FAT領域1個あたりのセクタ数
    uint32_t sectslong;         // +0x0c.l 全セクタ数 (long)
} __attribute__((__packed__, __aligned__(2)));

// Drive Parameter Block (remote drive common part)
struct dos_dpb_remote {
    int8_t drive;                       // +0x00.b ドライブ番号
    int8_t unit;                        // +0x01.b ユニット番号
    struct dos_devheader *devheader;    // +0x02.l デバイスヘッダへのポインタ
    struct dos_dpb *next;               // +0x06.l 次のDPBへのポインタ
    uint16_t sectbytes;                 // +0x0a.w 1セクタあたりのバイト数 (=0)
} __attribute__((__packed__, __aligned__(2)));

// Drive Parameter Block
struct dos_dpb {
    int8_t drive;                       // +0x00.b ドライブ番号
    int8_t unit;                        // +0x01.b ユニット番号
    struct dos_devheader *devheader;    // +0x02.l デバイスヘッダへのポインタ
    struct dos_dpb *next;               // +0x06.l 次のDPBへのポインタ
    uint16_t sectbytes;                 // +0x0a.w 1セクタあたりのバイト数

    uint8_t sectclust;                  // +0x0c.b 1クラスタあたりのセクタ数-1
    uint8_t csshift;                    // +0x0d.b クラスタ→セクタのシフト数
    uint16_t fatsect;                   // +0x0e.w FATの先頭セクタ番号
    uint8_t fatnum;                     // +0x10.b FATの数
    uint8_t fatsects;                   // +0x11.b 1個のFATあたりのセクタ数
    uint16_t rootent;                   // +0x12.w ルートディレクトリエントリ数
    uint16_t datasect;                  // +0x14.w データ領域の先頭セクタ番号
    uint16_t totalclu;                  // +0x16.w 総クラスタ数+1
    uint16_t rootsect;                  // +0x18.w ルートディレクトリの先頭セクタ番号
    uint8_t mediabyte;                  // +0x1a.b メディアバイト
    uint8_t sbshift;                    // +0x1b.b セクタ→バイトのシフト数
    uint16_t fatfindpos;                // +0x1c.w FAT検索開始位置

    uint32_t schdir_firstfat;           // +0x1e.l 空きエントリ検索先頭FAT番号
    uint16_t schdir_clusect;            // +0x22.w
    uint32_t schdir_nextsect;           // +0x24.l
    uint16_t schdir_remsect;            // +0x28.w

    uint32_t schfil_firstfat;           // +0x2a.l ファイル検索先頭FAT番号
    uint16_t schfil_clusect;            // +0x2e.w
    uint32_t schfil_nextsect;           // +0x30.l
    uint16_t schfil_remsect;            // +0x34.w
    uint16_t schfil_offset;             // +0x36.w
} __attribute__((__packed__, __aligned__(2)));

// Current Directory Table
struct dos_curdir {
    uint8_t drive;                      // +0x00.b 物理ドライブ名
    uint8_t coron;                      // +0x01.b コロン文字 (':')
    uint8_t path[62];                   // +0x02.b カレントディレクトリのパス (デリミタは'\t')
    uint32_t reserved1;                 // +0x40.l
    uint8_t reserved2;                  // +0x44.b
    uint8_t type;                       // +0x45.b ドライブ種別
    struct dos_dpb *dpb;                // +0x46.l DPBへのポインタ
    uint16_t curfat;                    // +0x4a.w
    uint16_t pathlen;                   // +0x4c.w
} __attribute__((__packed__, __aligned__(2)));

#endif /* _DOS_INTERNAL_H_ */
