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

#include <string.h>
#include <x68k/dos.h>
#include <x68k/tsr.h>
#include "tsr_internal.h"

// nameと一致する非デバイスドライバ型の常駐プロセスを検索する
//  name: 検索する常駐プロセスの名前
//  戻り値: 見つかった常駐プロセス。見つからない場合はNULL
tsr_process_t tsr_findproc(const char *name)
{
    int ssp = tsr_super(0);
    struct dos_mep *mep = (struct dos_mep *)MEMBLK_TOP;   // 先頭のメモリブロック

    do {
        tsr_process_t th = (tsr_process_t)((char *)mep + sizeof(struct dos_mep) + sizeof(struct dos_psp));
        if (((int)mep->parent_mp & 0xff000000) == 0xff000000) {
            if (th->signature == TSR_SIGNATURE &&
                strncmp(name, th->name, sizeof(th->name)) == 0) {
                tsr_super(ssp);
                return th;
            }
        }
    } while ((mep = mep->next_mp) != NULL);

    tsr_super(ssp);
    return NULL;
}
