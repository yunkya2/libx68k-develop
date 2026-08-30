/*
 * DOSコール、IOCSコール、CPU割り込みベクタをまとめてフックするサンプル。
 * 各ハンドラでは旧処理を呼び出してから、入力文字数や呼び出し回数を記録する。
 *
 * このファイルは登録処理と常駐する処理を分離していない。tsr_keepproc()には
 * NULLを渡すため、プログラムの静的領域末尾までがそのまま常駐領域になる。
 */

#include <stdint.h>
#include <stdio.h>
#include <string.h>
#include <x68k/dos_inline.h>
#include <x68k/tsr.h>

#define TSR_NAME "TSR VECTHOOK1"

enum {
    /* DOS/IOCSコールは、コール番号に対応するベクタを差し替える。 */
    DOS_GETS_VECTOR = 0xff0a,
    IOCS_B_KEYINP_VECTOR = 0x0100,
    /* 0x4cはキーボードから入力があったときに呼ばれるCPU割り込みベクタ。 */
    KEY_INPUT_VECTOR = 0x4c,
};

/* TSR_*HANDLER内のアセンブラから参照するため、Cハンドラは外部リンケージにする。 */
int dos_gets_handler(void *arg);
void iocs_keyinp_handler(struct iocs_regs *regs);
static void key_input_handler(void) __attribute__((__interrupt__));

/* 非デバイスドライバ型TSRを識別するヘッダを定義する。 */
TSR_PROCESS(TSR_NAME);
/* Human68kの呼出規約とC関数の引数形式を変換する入口を生成する。 */
TSR_DOSHANDLER(dos_gets_entry, dos_gets_handler);
TSR_IOCSHANDLER(iocs_keyinp_entry, iocs_keyinp_handler);

TSR_HEAP(1024);
TSR_NO_ENVIRON();

struct hook_data {
    /* oldvecにはtsr_setvectors()が差し替え前の処理アドレスを保存する。 */
    tsr_vecinfo_t vecs[4];
    /* ハンドラと通常処理の双方から参照されるためvolatileにする。 */
    volatile uint32_t dos_gets_chars;
    volatile uint32_t iocs_keyinp_count;
    volatile uint32_t key_input_count;
    volatile int iocs_last_key;
};

static struct hook_data hook_data = {
    .vecs = {
        { .vecno = DOS_GETS_VECTOR, .newvec = dos_gets_entry },
        { .vecno = IOCS_B_KEYINP_VECTOR, .newvec = iocs_keyinp_entry },
        { .vecno = KEY_INPUT_VECTOR, .newvec = key_input_handler },
        { 0 },  /* vecno == 0が配列の終端。 */
    },
};

/* 常駐後のハンドラは、同じ常駐領域に残ったhook_dataをこのポインタで参照する。 */
static struct hook_data *data = &hook_data;

int dos_gets_handler(void *arg)
{
    /* A6はDOS引数列を指す。_GETSの第1引数はその先頭にあるポインタ。 */
    struct dos_inpptr *inpptr = *(struct dos_inpptr **)arg;
    /* 先に元の_GETSを実行し、入力後のlengthを集計する。 */
    int result = tsr_doscall(data->vecs[0].oldvec, arg);

    data->dos_gets_chars += inpptr->length;
    return result;
}

void iocs_keyinp_handler(struct iocs_regs *regs)
{
    /* 保存されたレジスタで元のIOCS処理を呼び、戻り値D0を記録する。 */
    tsr_iocscall(data->vecs[1].oldvec, regs);
    data->iocs_last_key = regs->d0;
    data->iocs_keyinp_count++;
}

static void key_input_handler(void)
{
    /* 割り込み中はDOS/IOCSやstdioを呼ばず、旧処理の呼出しと加算だけを行う。 */
    tsr_interruptcall(data->vecs[2].oldvec);
    data->key_input_count++;
}

static void clear_counts(struct hook_data *resident)
{
    /* 割り込みハンドラと共有する値をまとめて変更する間、割り込みを禁止する。 */
    int ssp = tsr_super(0);
    uint16_t sr = tsr_saveirq();

    resident->dos_gets_chars = 0;
    resident->iocs_keyinp_count = 0;
    resident->key_input_count = 0;
    resident->iocs_last_key = 0;

    tsr_restoreirq(sr);
    tsr_super(ssp);
}

static void print_status(struct hook_data *resident)
{
    uint32_t dos_chars;
    uint32_t iocs_count;
    uint32_t interrupt_count;
    int iocs_last_key;
    char buffer[160];
    /* 表示中の更新は許し、表示用のスナップショット取得時だけ保護する。 */
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
    struct hook_data *resident;

    if (argc > 2 || (option != NULL && strcmp(option, "-s") != 0 &&
                     strcmp(option, "-c") != 0 && strcmp(option, "-r") != 0)) {
        _dos_print("usage: vecthook [-s|-c|-r]\r\n");
        return 1;
    }

    proc = tsr_findproc(TSR_NAME);
    if (option == NULL && proc == NULL) {
        /* 再実行したプロセスから管理データを取得できるようヘッダへ保存する。 */
        tsr_setprocdata(TSR_THISPROC, data);
        /* 3ベクタを差し替え、元のアドレスを各oldvecへ保存する。 */
        tsr_setvectors(data->vecs);
        _dos_print("vecthook installed\r\n");
        tsr_keepproc(NULL, 0);
    }

    if (proc == NULL) {
        _dos_print("vecthook is not installed\r\n");
        return 1;
    }
    /* procは常駐領域を指す。現在実行中のプログラム側のdataとは別物。 */
    resident = tsr_getprocdata(proc);

    if (option == NULL || strcmp(option, "-s") == 0) {
        print_status(resident);
        return 0;
    }
    if (strcmp(option, "-c") == 0) {
        clear_counts(resident);
        _dos_print("vecthook counters cleared\r\n");
        return 0;
    }

    /* 他のプログラムが後から変更していた場合は、チェインを壊さず解除を拒否する。 */
    if (tsr_restorevectors(resident->vecs) < 0) {
        _dos_print("vecthook cannot restore modified vectors\r\n");
        return 1;
    }
    if (tsr_freeproc(proc) < 0) {
        _dos_print("vecthook cannot be removed\r\n");
        return 1;
    }
    return 0;
}
