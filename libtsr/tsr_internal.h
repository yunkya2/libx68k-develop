/*
 * Copyright (c) 2026 Yuichi Nakamura (@yunkya2)
 *
 * The MIT License (MIT)
 *
 * Permission is hereby granted, free of charge, to any person obtaining a copy
 * of this software and associated documentation files (the "Software"), to deal
 * in the Software without restriction, including without limitation the rights
 * to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
 * copies of the Software, and to permit persons to whom the Software is
 * furnished to do so, subject to the following conditions:
 *
 * The above copyright notice and this permission notice shall be included in
 * all copies or substantial portions of the Software.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
 * IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
 * FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
 * AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
 * LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
 * OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN
 * THE SOFTWARE.
 */

#include <stdint.h>
#include <string.h>
#include <x68k/tsr.h>

//****************************************************************************
// Macros and definitions
//****************************************************************************

// Human68k work area
#define MEMBLK_TOP    (*(char **)0x1c20)                // 先頭のメモリブロック
#define CURDIR_TABLE  (*(struct dos_curdir **)0x1c38)   // カレントディレクトリテーブル
#define MAXFILES      (*(uint16_t *)0x1c6e)             // ファイルディスクリプタ最大値
#define BUFFERS_SEC   (*(uint16_t *)0x1c70)             // BUFFERSのセクタサイズ(第2引数)
#define LASTDRIVE     (*(uint8_t *)0x1c73)              // LASTDRIVEの値
#define CONNDRIVE     (*(uint8_t *)0x1c75)              // 接続ドライブ数-1
#define DRVXTBL       ((uint8_t *)0x1c7e)               // ドライブ交換テーブル

// x68k.ldが定義するlibtsr内部用シンボル
extern char _stext;
extern char _tsr_header_end;
extern char _tsr_end;

//****************************************************************************
// Internal functions
//****************************************************************************

// Human68kのNULデバイスヘッダを探す
static inline struct dos_devheader *find_nuldev(void)
{
    char *p = MEMBLK_TOP;
    while (memcmp(p, "NUL     ", 8) != 0) {
        p += 2;
    }
    return (struct dos_devheader *)(p - 14);
}
