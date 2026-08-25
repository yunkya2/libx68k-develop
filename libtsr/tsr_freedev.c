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

// devが常駐解除できるか調べる
//  dev: 調べるデバイス
//  戻り値: 0 解除できない
//          非0 解除できる
int tsr_isfreeable(tsr_device_t dev)
{
    if (dev == NULL) {
        return 0;
    }

    int ssp = tsr_super(0);
    int freeable = ((struct tsr_devheader_first *)dev)->memblock != NULL;
    tsr_super(ssp);
    return freeable;
}

// devをデバイスチェインから外し、常駐メモリを解放する
//  dev: 常駐解除するデバイス
//  戻り値: 0 成功
//          負値 解除できない場合やチェイン上にない場合
int tsr_freedev(tsr_device_t dev)
{
    if (dev == NULL) {
        return -1;
    }

    int ssp = tsr_super(0);
    struct tsr_devheader_first *header = (struct tsr_devheader_first *)dev;
    if (header->memblock == NULL) {
        tsr_super(ssp);
        return -1;
    }

    struct dos_devheader *prev = find_nuldev();
    while (prev->next != (struct dos_devheader *)-1 &&
           prev->next != &dev->header) {
        prev = prev->next;
    }
    if (prev->next != &dev->header) {
        tsr_super(ssp);
        return -1;
    }

    void *memblock = header->memblock;
    prev->next = header->tail->next;
    tsr_super(ssp);

    _dos_mfree(memblock);
    return 0;
}
