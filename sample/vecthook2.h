#ifndef VECTHOOK2_H
#define VECTHOOK2_H

#include <stdint.h>
#include <x68k/tsr.h>

#define VECTHOOK2_NAME "TSR VECTHOOK2"

/* 常駐側が更新し、非常駐側が表示・クリアする共有データ。 */
struct vecthook2_data {
    tsr_vecinfo_t vecs[4];
    volatile uint32_t dos_gets_chars;
    volatile uint32_t iocs_keyinp_count;
    volatile uint32_t key_input_count;
    volatile int iocs_last_key;
};

/* 実体はvecthook2tsr.cにあり、tsrgenが常駐オブジェクトへ格納する。 */
extern struct tsr_process TSR_PROCHEADER_ID;
extern struct vecthook2_data vecthook2_data;

#endif