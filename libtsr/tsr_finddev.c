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
#include <x68k/tsr.h>
#include "tsr_internal.h"

// nameと一致する常駐デバイスを検索する
//  name: 検索する常駐デバイスの名前
//  戻り値: 見つかったデバイス。見つからない場合またはnameがNULLの場合はNULL
tsr_device_t tsr_finddev(const char *name)
{
    if (name == NULL) {
        return NULL;
    }

    int ssp = tsr_super(0);
    struct dos_devheader *devh = find_nuldev();
    while (devh != (struct dos_devheader *)-1) {
        struct tsr_devheader_first *first =
            (struct tsr_devheader_first *)devh;
        if (memcmp(devh->name, name, 8) == 0 &&
            first->signature == TSR_SIGNATURE) {
            tsr_super(ssp);
            return (tsr_device_t)devh;
        }
        devh = devh->next;
    }

    tsr_super(ssp);
    return NULL;
}
