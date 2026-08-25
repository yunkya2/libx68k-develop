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

// DPBに格納された実ドライブ番号を、ユーザー向けドライブ番号に変換する
//  realdrive: 実ドライブ番号
//  戻り値: ユーザー向けドライブ番号 (0=A, 1=B, ...)
//          TSR_ERR_NOTFOUND 対応するドライブがない
int tsr_getlogicaldrive(int realdrive)
{
    int ssp = tsr_super(0);
    for (int drive = 0; drive < 26; drive++) {
        if (DRVXTBL[drive] == realdrive) {
            tsr_super(ssp);
            return drive;
        }
    }
    tsr_super(ssp);
    return TSR_ERR_NOTFOUND;
}
