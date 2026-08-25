/*
 * vecthook2の常駐側。
 * ベクタ入口、ハンドラ、カウンタなど、常駐後も必要なものだけを置く。
 */

#include <x68k/dos.h>
#include "vecthook2.h"

enum {
    DOS_GETS_VECTOR = 0xff0a,
    IOCS_B_KEYINP_VECTOR = 0x0100,
    KEY_INPUT_VECTOR = 0x4c,
};

int vecthook2_dos_gets_handler(void *arg);
void vecthook2_iocs_keyinp_handler(struct iocs_regs *regs);
static void vecthook2_key_input_handler(void) __attribute__((interrupt));

TSR_PROCESS(VECTHOOK2_NAME);
TSR_DOSHANDLER(vecthook2_dos_gets_entry, vecthook2_dos_gets_handler);
TSR_IOCSHANDLER(vecthook2_iocs_keyinp_entry, vecthook2_iocs_keyinp_handler);

/* ヒープと環境初期化の置換も、常駐オブジェクトへ含める。 */
TSR_HEAP(1024);
TSR_NO_ENVIRON();

struct vecthook2_data vecthook2_data = {
    .vecs = {
        { .vecno = DOS_GETS_VECTOR, .newvec = vecthook2_dos_gets_entry },
        { .vecno = IOCS_B_KEYINP_VECTOR,
          .newvec = vecthook2_iocs_keyinp_entry },
        { .vecno = KEY_INPUT_VECTOR,
          .newvec = vecthook2_key_input_handler },
        { 0 },
    },
};

int vecthook2_dos_gets_handler(void *arg)
{
    struct dos_inpptr *inpptr = *(struct dos_inpptr **)arg;
    /* フック内では表示せず、旧処理の結果を常駐データへ記録する。 */
    int result = tsr_doscall(vecthook2_data.vecs[0].oldvec, arg);

    vecthook2_data.dos_gets_chars += inpptr->length;
    return result;
}

void vecthook2_iocs_keyinp_handler(struct iocs_regs *regs)
{
    tsr_iocscall(vecthook2_data.vecs[1].oldvec, regs);
    vecthook2_data.iocs_last_key = regs->d0;
    vecthook2_data.iocs_keyinp_count++;
}

static void vecthook2_key_input_handler(void)
{
    tsr_interruptcall(vecthook2_data.vecs[2].oldvec);
    vecthook2_data.key_input_count++;
}