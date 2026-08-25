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

#include <x68k/tsr.h>
#include "tsr_internal.h"

// ブロックデバイスを接続する
//  drive: 接続する先頭ドライブ番号 (-1=空きドライブを使用 / 0=A, 1=B, ...)
//  units: ユニット数
//  dpbs: DPB構造体テーブルへのポインタ
//  dev: 接続するデバイス
//  戻り値: 0 成功
//          TSR_ERR_INUSE 指定されたドライブ番号が使用中
//          TSR_ERR_NOSPACE 空きドライブが不足
int tsr_attachdrive(int drive, int units, struct dos_dpb **dpbs, tsr_device_t dev)
{
    int ssp = tsr_super(0);

    // 空きドライブ数が足りるか確認する
    int drv = (drive < 0) ? 0 : drive;
    int freedrive = 0;
    while (drv < LASTDRIVE) {
        struct dos_curdir *curdir = &CURDIR_TABLE[(int)DRVXTBL[drv]];
        if (curdir->type == 0) {
            freedrive++;  // 空きドライブ
        } else {
            if (drv == drive) {
                tsr_super(ssp);
                return TSR_ERR_INUSE;
            }
        }
        drv++;
    }
    if (freedrive < units) {
        tsr_super(ssp);
        return TSR_ERR_NOSPACE;
    }

    // 先頭から空きドライブを探して接続する
    drv = (drive < 0) ? 0 : drive;
    int unit = 0;
    while (unit < units && drv < LASTDRIVE) {
        int realdrv = DRVXTBL[drv]; // 実ドライブ番号に変換
        drv++;

        // 空きドライブかどうか確認する
        struct dos_curdir *curdir = &CURDIR_TABLE[realdrv];
        if (curdir->type != 0) {
            continue;  // 使用中のドライブ
        }

        // DPBを初期化
        struct dos_dpb *dpb = dpbs[unit];
        dpb->unit = unit;                   // ユニット番号
        dpb->drive = realdrv;               // 実ドライブ番号
        dpb->devheader = &dev->header;       // デバイスヘッダへのポインタ
        dpb->next = (struct dos_dpb *)-1;   // 次のDPBへのポインタ
        unit++;

        // Human68kのDPBリストにDPBを繋ぐ
        struct dos_dpb *prev_dpb = NULL;
        for (int i = 0; i < realdrv; i++) {     // 1つ前の実ドライブのDPBを探す
            if (CURDIR_TABLE[i].type == 0x40) {
                prev_dpb = CURDIR_TABLE[i].dpb;
            }
        }
        if (prev_dpb != NULL) {
            dpb->next = prev_dpb->next;     // 次のDPBへのポインタを繋ぐ
            prev_dpb->next = dpb;
        }

        // Human68kのカレントディレクトリテーブルを設定する
        curdir->drive = 'A' + realdrv;
        curdir->coron = ':';
        curdir->path[0] = '\t';
        curdir->path[1] = '\0';
        curdir->type = 0x40;
        curdir->dpb = dpb;
        curdir->curfat = (int)-1;
        curdir->pathlen = 2;

        // 接続ドライブ数を増加
        CONNDRIVE++;
    }

    tsr_super(ssp);
    return 0;
}
