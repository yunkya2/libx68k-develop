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

// 複数のベクタをまとめて変更前のアドレスに復帰する
//  vecs: ベクタ情報配列 (vecnoが0の要素で終端)
//  戻り値: 0 復帰成功
//          -1 復帰できないベクタがあった
int tsr_restorevectors(struct tsr_vecinfo *vecs)
{
    for (int i = 0; vecs[i].vecno != 0; i++) {
        if (_dos_intvcg(vecs[i].vecno) != vecs[i].newvec) {
            return -1;
        }
    }

    for (int i = 0; vecs[i].vecno != 0; i++) {
        _dos_intvcs(vecs[i].vecno, vecs[i].oldvec);
    }
    return 0;
}
