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

// デバイスドライバ初期化コマンド(0x00)で渡されるパラメータをargc, argv形式に変換する
//  param: デバイスドライバ初期化コマンドで渡されたパラメータ
//  argv:  変換後の文字列ポインタ配列 (末尾はNULLで終端する)
//  maxargv: 終端のNULLを含めてargvに格納できる最大ポインタ数
//  戻り値: 格納した引数の数
int tsr_parseinitparam(void *param, char **argv, int maxargv)
{
    char *p = (char *)param;
    int argc = 0;

    if (maxargv <= 0) {
        return 0;
    }

    while (*p != '\0' && argc < maxargv - 1) {
        argv[argc++] = p;
        p += strlen(p) + 1;
    }
    argv[argc] = NULL;
    return argc;
}
