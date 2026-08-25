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

#include <x68k/dos.h>

#include <x68k/tsr.h>
#include "tsr_internal.h"

// 指定したデバイスががオープンされているファイルに使われていないか確認する
static int check_device_busy(struct dos_devheader *devheader)
{
    int fd = 0;

    while (1) {
        union dos_fcb *fcb = _dos_get_fcb_adr(fd++);
        if ((int)fcb == _DOSE_BADF) {
            continue;   // ファイルハンドルはオープンされていない
        } else if ((int)fcb < 0) {
            break;      // 全ファイルハンドルを確認した
        }
            if (fcb->chr.devattr & 0x80) {  // キャラクタデバイス
            if (fcb->chr.deventry == devheader) {
                    return TSR_ERR_BUSY;
            }
        } else {  // ブロックデバイス
            if (((struct dos_dpb *)fcb->blk.deventry)->devheader == devheader) {
                    return TSR_ERR_BUSY;
            }
        }
    }

    return 0;
}

// ブロックデバイスを切断する
//  dev: 切断するデバイス
//  戻り値: 0 成功
//          TSR_ERR_NODEV デバイスが存在しない
//          TSR_ERR_BUSY デバイスが使用中
int tsr_detachdrive(tsr_device_t dev)
{
    if (dev == NULL) {
    return TSR_ERR_NODEV;
    }

    int ssp = tsr_super(0);

    if (check_device_busy(&dev->header) < 0) {
        tsr_super(ssp);
        return TSR_ERR_BUSY;
    }

    for (int drv = 0; drv < 26; drv++) {
        // 削除対象のドライブかどうか確認する
        struct dos_curdir *curdir = &CURDIR_TABLE[(int)DRVXTBL[drv]];
        if (curdir->type != 0x40 || curdir->dpb->devheader != &dev->header) {
            continue;
        }

        // カレントディレクトリをルートに戻す (I/Oバッファもフラッシュ)
        if (_dos_drvctrl(9, drv + 1) < 0) {
            tsr_super(ssp);
            return TSR_ERR_BUSY;  // 使用中でないことはチェック済みなので来ないはず
        }

        // カレントディレクトリテーブルを未使用に戻してDPBをリンクリストから外す
        curdir->type = 0;
        for (int i = 0; i < 26; i++) {
            if (CURDIR_TABLE[i].type == 0x40 &&
                CURDIR_TABLE[i].dpb->next == curdir->dpb) {
                CURDIR_TABLE[i].dpb->next = curdir->dpb->next;
            }
        }

        // 接続ドライブ数を減少
        CONNDRIVE--;
    }

    tsr_super(ssp);
    return 0;
}
