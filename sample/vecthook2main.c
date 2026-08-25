/*
 * vecthook2の非常駐側。
 * 引数処理、登録、状態表示、カウンタのクリア、常駐解除を担当する。
 */

#include <stdint.h>
#include <stdio.h>
#include <string.h>
#include <x68k/dos_inline.h>
#include "vecthook2.h"

static void clear_counts(struct vecthook2_data *resident)
{
    int ssp = tsr_super(0);
    uint16_t sr = tsr_saveirq();

    resident->dos_gets_chars = 0;
    resident->iocs_keyinp_count = 0;
    resident->key_input_count = 0;
    resident->iocs_last_key = 0;

    tsr_restoreirq(sr);
    tsr_super(ssp);
}

static void print_status(struct vecthook2_data *resident)
{
    uint32_t dos_chars;
    uint32_t iocs_count;
    uint32_t interrupt_count;
    int iocs_last_key;
    char buffer[160];
    int ssp = tsr_super(0);
    uint16_t sr = tsr_saveirq();

    dos_chars = resident->dos_gets_chars;
    iocs_count = resident->iocs_keyinp_count;
    interrupt_count = resident->key_input_count;
    iocs_last_key = resident->iocs_last_key;

    tsr_restoreirq(sr);
    tsr_super(ssp);

    sprintf(buffer,
            "DOS _GETS chars : %lu\r\n"
            "IOCS _B_KEYINP: %lu (last=%08lx)\r\n"
            "key interrupt: %lu\r\n",
            (unsigned long)dos_chars,
            (unsigned long)iocs_count, (unsigned long)iocs_last_key,
            (unsigned long)interrupt_count);
    _dos_print(buffer);
}

int main(int argc, char **argv)
{
    const char *option = argc == 2 ? argv[1] : NULL;
    tsr_process_t proc;
    struct vecthook2_data *resident;

    if (argc > 2 || (option != NULL && strcmp(option, "-s") != 0 &&
                     strcmp(option, "-c") != 0 && strcmp(option, "-r") != 0)) {
        _dos_print("usage: vecthook2 [-s|-c|-r]\r\n");
        return 1;
    }

    proc = tsr_findproc(VECTHOOK2_NAME);
    if (option == NULL && proc == NULL) {
        /* 常駐側のヘッダとデータを初期化してからベクタを差し替える。 */
        tsr_setprocdata(TSR_THISPROC, &vecthook2_data);
        tsr_setvectors(vecthook2_data.vecs);
        _dos_print("vecthook2 installed\r\n");
        /* NULLならtsrgenが生成した常駐部末尾までを残す。 */
        tsr_keepproc(NULL, 0);
    }

    if (proc == NULL) {
        _dos_print("vecthook2 is not installed\r\n");
        return 1;
    }
    /* 新しくロードされた側ではなく、常駐側オブジェクト内のデータを取得する。 */
    resident = tsr_getprocdata(proc);

    if (option == NULL || strcmp(option, "-s") == 0) {
        print_status(resident);
        return 0;
    }
    if (strcmp(option, "-c") == 0) {
        clear_counts(resident);
        _dos_print("vecthook2 counters cleared\r\n");
        return 0;
    }

    if (tsr_restorevectors(resident->vecs) < 0) {
        _dos_print("vecthook2 cannot restore modified vectors\r\n");
        return 1;
    }
    if (tsr_freeproc(proc) < 0) {
        _dos_print("vecthook2 cannot be removed\r\n");
        return 1;
    }
    return 0;
}