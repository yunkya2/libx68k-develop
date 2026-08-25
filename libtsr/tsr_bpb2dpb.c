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

#include <x68k/iocs.h>

#include <x68k/tsr.h>
#include "tsr_internal.h"

// BPBからDPBを構築する
//  bpb: BPB構造体へのポインタ
//  dpb: DPB構造体へのポインタ
//  戻り値: 0 成功
//          TSR_ERR_SECTSIZE セクタサイズが大きすぎる
//          TSR_ERR_CLUSTERS クラスタ数が多すぎる
int tsr_bpb2dpb(struct dos_bpb *bpb, struct dos_dpb *dpb)
{
    int bytes;
    int shift;

    // I/Oバッファのセクタサイズより大きいセクタならエラー
    if (_iocs_b_wpeek(&BUFFERS_SEC) < bpb->sectbytes) {
        return TSR_ERR_SECTSIZE;
    }
    dpb->sectbytes = bpb->sectbytes;        // 1セクタあたりのバイト数

    // セクタサイズからセクタ→バイトのシフト数を計算する
    bytes = bpb->sectbytes - 1;
    shift = 0;
    while (bytes > 0) {
        bytes >>= 1;
        shift++;
      }
    dpb->sbshift = shift;                   // セクタ→バイトのシフト数

    dpb->sectclust = bpb->sectclust - 1;    // 1クラスタあたりのセクタ数-1

    // 1クラスタあたりのセクタ数からクラスタ→セクタのシフト数を計算する
    bytes = bpb->sectclust - 1;
    shift = 0;
    while (bytes > 0) {
        bytes >>= 1;
        shift++;
    }
    if (bpb->fatnum & 0x80) {
        shift |= 0x80;  // Intel FATの場合はbit7を立てる
    }
    dpb->csshift = shift;                   // クラスタ→セクタのシフト数

    dpb->fatnum = bpb->fatnum & 0x7f;       // FATの数
    dpb->fatsect = bpb->resvsects;          // FATの先頭セクタ番号
    dpb->fatsects = bpb->fatsects;          // 1個のFATあたりのセクタ数
    dpb->rootent = bpb->rootent;            // ルートディレクトリエントリ数

    // ルートディレクトリの先頭セクタ番号を計算する
    dpb->rootsect = dpb->fatsects * dpb->fatnum + dpb->fatsect;

    // ルートディレクトリのセクタ数を計算する
    int rootsects = (dpb->rootent * 32 + dpb->sectbytes - 1) / dpb->sectbytes;

    dpb->datasect = dpb->rootsect + rootsects;  // データ領域の先頭セクタ番号

    // 総クラスタ数を計算する
    int sects = bpb->sects ? bpb->sects : bpb->sectslong;
    dpb->totalclu = ((sects - dpb->datasect) >> dpb->csshift) + 3;  // 総クラスタ数+1
    if (dpb->totalclu >= 0xfff8) {
        return TSR_ERR_CLUSTERS;
    }

    dpb->mediabyte = bpb->mediabyte;        // メディアバイト
    dpb->fatfindpos = 2;                    // FAT検索開始位置(初期値は2)

    dpb->schdir_firstfat = 0;   // 空きエントリ検索先頭FAT番号
    dpb->schfil_firstfat = 0;   // ファイル検索先頭FAT番号

    return 0;
}
