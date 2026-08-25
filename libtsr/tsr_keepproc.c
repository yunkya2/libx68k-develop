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

#include <x68k/dos_inline.h>
#include <x68k/tsr.h>
#include "tsr_internal.h"

// 非デバイスドライバ型プログラムを常駐させ、終了コードcodeで終了する
//  tsrend: 常駐部の終了アドレス (NULLなら既定の常駐終了アドレスを使う)
//  code: 終了コード
//  戻り値: なし (この関数は戻らない)
__attribute__((noreturn))
void tsr_keepproc(void *tsrend, int code)
{
    extern struct tsr_process TSR_PROCHEADER_ID;

    TSR_PROCHEADER_ID.memblock = _dos_getpdb();
    if (tsrend == NULL) {
        tsrend = tsr_getresidentend();
    }
    _dos_keeppr((int)tsrend - (int)&_stext, code);
}
