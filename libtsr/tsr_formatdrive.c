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

// ブロックデバイスをFATでフォーマットする
//  dpb: DPB構造体へのポインタ
//  preparesect: 初期化対象セクタの書き込み可能なバッファを用意する関数
//  commitsect: 初期化したバッファをセクタに反映する関数(NULLでも可)
//  arg: preparesect, commitsectに渡される引数
//  戻り値: 0 成功
//          TSR_ERR_NOBUF セクタバッファを取得できない
//          その他の負値 commitsectが返したエラー
int tsr_formatdrive(struct dos_dpb *dpb, tsr_preparesect_t *preparesect,
                    tsr_commitsect_t *commitsect, void *arg)
{
    // FATを初期化する
    for (int i = 0; i < dpb->fatnum; i++) {
        for (int j = 0; j < dpb->fatsects; j++) {
            int sect = dpb->fatsect + i * dpb->fatsects + j;
            uint8_t *fat = preparesect(arg, sect);
            if (fat == NULL) {
                return TSR_ERR_NOBUF;
            }
            memset(fat, 0, dpb->sectbytes);
            if (j == 0) {
                fat[0] = dpb->mediabyte;
                fat[1] = 0xff;
                fat[2] = 0xff;
                if (dpb->totalclu >= 0x0ff8) {
                    fat[3] = 0xff;  // FAT16の場合
                }
            }
            if (commitsect != NULL) {
                int res = commitsect(arg, sect, fat);
                if (res < 0) {
                    return res;
                }
            }
        }
    }

    // ルートディレクトリを初期化する
    for (int i = dpb->rootsect; i < dpb->datasect; i++) {
        uint8_t *dir = preparesect(arg, i);
        if (dir == NULL) {
            return TSR_ERR_NOBUF;
        }
        memset(dir, 0, dpb->sectbytes);
        if (commitsect != NULL) {
            int res = commitsect(arg, i, dir);
            if (res < 0) {
                return res;
            }
        }
    }

    return 0;
}
