#ifndef _TSR_H_
#define _TSR_H_

#include <stdint.h>
#include <stddef.h>
#include <sys/cdefs.h>
#include <x68k/dos_internal.h>
#include <x68k/iocs.h>

__BEGIN_DECLS

// libtsr signature
#define TSR_SIGNATURE   0x23545352      // "#TSR"

//****************************************************************************
// Device driver definitions and API
//****************************************************************************

struct tsr_devheader {
    struct dos_devheader header;
    void        *data;          // +0x16.l  デバイス固有データ
} __attribute__((packed, aligned(2)));
typedef struct tsr_devheader *tsr_device_t;

struct tsr_devheader_first {
    struct tsr_devheader devheader;
    uint32_t    signature;      // +0x1a.l  TSR_SIGNATURE
    void        *memblock;      // +0x1e.l  常駐メモリブロック
    struct dos_devheader *tail; // +0x22.l  末尾のデバイスヘッダ
} __attribute__((packed, aligned(2)));

#define TSR_DEVHEADER_ID(_id)       _devheader_ ## _id
#define TSR_DEVHEADER_ID_STR(_id)   "_devheader_" #_id

#define TSR_DEFDEVHEADER_FIRST(_attr, _name, _next, _tail) \
    extern void _strategy_asm_(void); \
    extern void _interrupt_asm_(void); \
    __attribute__((section(".header"))) \
    struct tsr_devheader_first TSR_DEVHEADER_ID() = { \
        { { (struct dos_devheader *)(_next), (_attr), \
              (void *)_strategy_asm_, (void *)_interrupt_asm_, \
              { (_name)[0], (_name)[1], (_name)[2], (_name)[3], \
                (_name)[4], (_name)[5], (_name)[6], (_name)[7] } }, 0 }, \
        TSR_SIGNATURE, 0, (struct dos_devheader *)(_tail) \
    }

#define TSR_DEFDEVHEADER(_attr, _name, _id, _next) \
    extern void _strategy_asm_ ## _id(void); \
    extern void _interrupt_asm_ ## _id(void); \
    __attribute__((section(".header.1"))) \
    struct tsr_devheader TSR_DEVHEADER_ID(_id) = { \
        { (struct dos_devheader *)(_next), (_attr), \
          (void *)_strategy_asm_ ## _id, (void *)_interrupt_asm_ ## _id, \
          { (_name)[0], (_name)[1], (_name)[2], (_name)[3], \
            (_name)[4], (_name)[5], (_name)[6], (_name)[7] } }, 0 \
    }

#define TSR_ASMSERVICES(_interrupt, _id) \
    __asm__ ( \
        "       .section \".header.asm\",\"ax\"\n" \
        "       .global _strategy_asm_" #_id "\n" \
        "_strategy_asm_" #_id ":\n" \
        "       move.l  %a5,_reqheader_" #_id "\n" \
        "       rts\n\n" \
        "_reqheader_" #_id ": .long 0\n\n" \
        "       .global _interrupt_asm_" #_id "\n" \
        "_interrupt_asm_" #_id ":\n" \
        "       movem.l %d0-%d2/%a0-%a2,%sp@-\n" \
        "       pea.l   %pc@(" TSR_DEVHEADER_ID_STR(_id) ")\n" \
        "       move.l  %pc@(_reqheader_" #_id "),%sp@-\n" \
        "       jbsr    " #_interrupt "\n" \
        "       movea.l %sp@+,%a0\n" \
        "       addq.l  #4,%sp\n" \
        "       move.b  %d0,%a0@(3)\n" \
        "       ror.w   #8,%d0\n" \
        "       move.b  %d0,%a0@(4)\n" \
        "       movem.l %sp@+,%d0-%d2/%a0-%a2\n" \
        "       rts\n\n" \
    )

// 現在のデバイスドライバのデバイスヘッダを取得する
#define TSR_THISDEV                 (&TSR_DEVHEADER_ID().devheader)
#define TSR_THISDEV_ID(_id)         (&TSR_DEVHEADER_ID(_id))

// 単一デバイスで構成するデバイスドライバを定義する
//  _attr: デバイス属性
//  _name: 8文字のデバイス名
//  _interrupt: リクエスト処理関数
#define TSR_DEVICE(_attr, _name, _interrupt) \
    TSR_DEFDEVHEADER_FIRST(_attr, _name, -1, &TSR_DEVHEADER_ID().devheader.header); \
    TSR_ASMSERVICES(_interrupt,)

// 複数デバイスで構成するデバイスドライバの先頭デバイスを定義する
//  _attr: デバイス属性
//  _name: 8文字のデバイス名
//  _interrupt: リクエスト処理関数
//  _next: 次のデバイスの識別子
//  _tail: 末尾のデバイスの識別子
#define TSR_DEVICE_FIRST(_attr, _name, _interrupt, _next, _tail) \
    extern struct tsr_devheader TSR_DEVHEADER_ID(_next); \
    extern struct tsr_devheader TSR_DEVHEADER_ID(_tail); \
    TSR_DEFDEVHEADER_FIRST(_attr, _name, &TSR_DEVHEADER_ID(_next).header, &TSR_DEVHEADER_ID(_tail).header); \
    TSR_ASMSERVICES(_interrupt,)

// 複数デバイスで構成するデバイスドライバの中間デバイスを定義する
//  _attr: デバイス属性
//  _name: 8文字のデバイス名
//  _interrupt: リクエスト処理関数
//  _id: このデバイスの識別子
//  _next: 次のデバイスの識別子
#define TSR_DEVICE_NEXT(_attr, _name, _interrupt, _id, _next) \
    extern struct tsr_devheader TSR_DEVHEADER_ID(_next); \
    TSR_DEFDEVHEADER(_attr, _name, _id, &TSR_DEVHEADER_ID(_next).header); \
    TSR_ASMSERVICES(_interrupt, _id)

// 複数デバイスで構成するデバイスドライバの末尾デバイスを定義する
//  _attr: デバイス属性
//  _name: 8文字のデバイス名
//  _interrupt: リクエスト処理関数
//  _id: このデバイスの識別子
#define TSR_DEVICE_TAIL(_attr, _name, _interrupt, _id) \
    TSR_DEFDEVHEADER(_attr, _name, _id, -1); \
    TSR_ASMSERVICES(_interrupt, _id)

// nameと一致する常駐デバイスを検索する
//  name: 検索する常駐デバイスの名前
//  戻り値: 見つかったデバイス。見つからない場合またはnameがNULLの場合はNULL
tsr_device_t tsr_finddev(const char *name);

// デバイスドライバを常駐させ、終了コードcodeでプロセスを終了する
//  tsrend: 常駐部の終了アドレス (NULLなら既定の常駐終了アドレスを使う)
//  code: 終了コード
//  戻り値: なし (この関数は戻らない)
__attribute__((noreturn))
void tsr_keepdev(void *tsrend, int code);

// 既定の常駐範囲の終了アドレスを返す
// (非常駐部分離型なら常駐部末尾、それ以外はプログラムの静的領域末尾を返す)
//  戻り値: 常駐範囲の終了アドレス
void *tsr_getresidentend(void);

// devが常駐解除できるか調べる
//  dev: 調べるデバイス
//  戻り値: 0 解除できない
//          非0 解除できる
int tsr_isfreeable(tsr_device_t dev);

// devをデバイスチェインから外し、常駐メモリを解放する
//  dev: 常駐解除するデバイス
//  戻り値: 0 成功
//          負値 解除できない場合やチェイン上にない場合
int tsr_freedev(tsr_device_t dev);

// devのデバイス固有データをdataに設定する
//  dev: デバイス (NULLなら何もしない)
//  data: 設定するデバイス固有データ
//  戻り値: なし
void tsr_setdevdata(tsr_device_t dev, void *data);

// devのデバイス固有データを返す
//  dev: デバイス
//  戻り値: デバイス固有データ。devがNULLの場合はNULL
void *tsr_getdevdata(tsr_device_t dev);

// デバイスドライバ初期化コマンド(0x00)で渡されるパラメータをargc, argv形式に変換する
//  param: デバイスドライバ初期化コマンドで渡されたパラメータ
//  argv:  変換後の文字列ポインタ配列 (末尾はNULLで終端する)
//  maxargv: 終端のNULLを含めてargvに格納できる最大ポインタ数
//  戻り値: 格納した引数の数
int tsr_parseinitparam(void *param, char **argv, int maxargv);


//****************************************************************************
// Resident process definitions and API
//****************************************************************************

struct tsr_process {
    uint32_t    signature;      // +0x00.l  TSR_SIGNATURE
    char        name[16];       // +0x04.b  TSR名
    void        *memblock;      // +0x14.l  常駐メモリブロック
    void        *data;          // +0x18.l  TSR固有データ
} __attribute__((packed, aligned(2)));
typedef struct tsr_process *tsr_process_t;

#define TSR_PROCHEADER_ID   _procheader_

// 現在の常駐プロセスのプロセスヘッダを取得する
#define TSR_THISPROC        (&TSR_PROCHEADER_ID)

// 非デバイスドライバ型の常駐プロセスを定義する
//  _name: 常駐プロセス名
#define TSR_PROCESS(_name) \
    __attribute__((section(".header"))) \
    struct tsr_process TSR_PROCHEADER_ID = { \
        TSR_SIGNATURE, (_name), 0, 0 \
    }

// nameと一致する非デバイスドライバ型の常駐プロセスを検索する
//  name: 検索する常駐プロセスの名前
//  戻り値: 見つかった常駐プロセス。見つからない場合はNULL
tsr_process_t tsr_findproc(const char *name);

// 非デバイスドライバ型プログラムを常駐させ、終了コードcodeで終了する
//  tsrend: 常駐部の終了アドレス (NULLなら既定の常駐終了アドレスを使う)
//  code: 終了コード
//  戻り値: なし (この関数は戻らない)
__attribute__((noreturn))
void tsr_keepproc(void *tsrend, int code);

// procの常駐メモリを解放する
//  proc: 解放する常駐プロセス
//  戻り値: 0 成功
//          -1 解放できない
int tsr_freeproc(tsr_process_t proc);

// procの常駐プロセス固有データをdataに設定する
//  proc: 常駐プロセス (NULLなら何もしない)
//  data: 設定する常駐プロセス固有データ
//  戻り値: なし
void tsr_setprocdata(tsr_process_t proc, void *data);

// procの常駐プロセス固有データを返す
//  proc: 常駐プロセス
//  戻り値: 常駐プロセス固有データ。procがNULLの場合はNULL
void *tsr_getprocdata(tsr_process_t proc);


//****************************************************************************
// Block device support API
//****************************************************************************

#define TSR_ERR_SECTSIZE   (-1)
#define TSR_ERR_CLUSTERS   (-2)
#define TSR_ERR_INUSE      (-3)
#define TSR_ERR_NOSPACE    (-4)
#define TSR_ERR_NOTFOUND   (-5)
#define TSR_ERR_NODEV      (-6)
#define TSR_ERR_BUSY       (-7)
#define TSR_ERR_NOBUF      (-8)

// BPBからDPBを構築する
//  bpb: BPB構造体へのポインタ
//  dpb: DPB構造体へのポインタ
//  戻り値: 0 成功
//          TSR_ERR_SECTSIZE セクタサイズが大きすぎる
//          TSR_ERR_CLUSTERS クラスタ数が多すぎる
int tsr_bpb2dpb(struct dos_bpb *bpb, struct dos_dpb *dpb);

// ブロックデバイスを接続する
//  drive: 接続する先頭ドライブ番号 (-1=空きドライブを使用 / 0=A, 1=B, ...)
//  units: ユニット数
//  dpbs: DPB構造体テーブルへのポインタ
//  dev: 接続するデバイス
//  戻り値: 0 成功
//          TSR_ERR_INUSE 指定されたドライブ番号が使用中
//          TSR_ERR_NOSPACE 空きドライブが不足
int tsr_attachdrive(int drive, int units, struct dos_dpb **dpbs, tsr_device_t dev);

// DPBに格納された実ドライブ番号を、ユーザー向けドライブ番号に変換する
//  realdrive: 実ドライブ番号
//  戻り値: ユーザー向けドライブ番号 (0=A, 1=B, ...)
//          TSR_ERR_NOTFOUND 対応するドライブがない
int tsr_getlogicaldrive(int realdrive);

// ブロックデバイスを切断する
//  dev: 切断するデバイス
//  戻り値: 0 成功
//          TSR_ERR_NODEV デバイスが存在しない
//          TSR_ERR_BUSY デバイスが使用中
int tsr_detachdrive(tsr_device_t dev);

// 初期化対象セクタの書き込み可能なバッファを用意する関数型
//  arg: 呼び出し元から渡される任意のデータ
//  sect: セクタ番号
//  戻り値: 書き込み可能なセクタバッファ。取得できない場合はNULL
typedef void *tsr_preparesect_t(void *arg, int sect);

// 初期化したバッファの内容を対象セクタに反映する関数型
//  arg: 呼び出し元から渡される任意のデータ
//  sect: セクタ番号
//  buf: 初期化したセクタバッファ
//  戻り値: 0以上 成功、負値 失敗
typedef int tsr_commitsect_t(void *arg, int sect, void *buf);

// ブロックデバイスをFATでフォーマットする
//  dpb: DPB構造体へのポインタ
//  preparesect: 初期化対象セクタの書き込み可能なバッファを用意する関数
//  commitsect: 初期化したバッファをセクタに反映する関数(NULLでも可)
//  arg: preparesect, commitsectに渡される引数
//  戻り値: 0 成功
//          TSR_ERR_NOBUF セクタバッファを取得できない
//          その他の負値 commitsectが返したエラー
int tsr_formatdrive(struct dos_dpb *dpb, tsr_preparesect_t *preparesect,
                    tsr_commitsect_t *commitsect, void *arg);

//****************************************************************************
// Heap and stack management API
//****************************************************************************

// 常駐プログラム用のヒープを定義する
//  _size: ヒープサイズ (バイト)
#define TSR_HEAP(_size) \
    __attribute__((aligned(4))) \
    char __heap[_size]; \
    char *_HSTA = __heap; \
    char *_HEND = __heap + _size; \
    char *_HMAX = __heap + _size;

// 常駐プログラム用のスタックを定義する
//  _size: スタックサイズ (バイト)
#define TSR_STACK(_size) \
    __attribute__((aligned(4))) \
    char __stack[_size]; \
    char *_SSTA = __stack; \
    char *_SEND = __stack + _size;

// ヒープの残りバイト数(現在のヒープ末尾から_HENDまで)を取得する
//  戻り値: ヒープの残りバイト数
size_t tsr_heapfree(void);

// プロセス起動時に環境変数の取り込みを行わない
#define TSR_NO_ENVIRON()        void __crt1_setup_environ(void) {}

// 常駐時にthreadやsocketをcloseされないようにする
#define TSR_KEEP_SOCKET()       void __socket_register_at_exit(void) {}
#define TSR_KEEP_PTHREAD()      void __pthread_register_at_exit(void) {}


//****************************************************************************
// DOS/IOCS vector and interrupt management API
//****************************************************************************

// スーパーバイザ/ユーザモードの切り替えを行う (ネスト可能)
//  ssp: 0ならスーパーバイザモードへ切り替え、非0ならそのSSPでユーザモードに復帰
//  戻り値: 切り替え前のSSP (既にスーパーバイザモードだった場合は0)
static inline int tsr_super(int ssp)
{
    int ret = _iocs_b_super(ssp);
    return (ret == -1) ? 0 : ret;
}

// CPU割り込みを禁止する (ネスト可能)
//  戻り値: 割り込み禁止前のSR
static inline uint16_t tsr_saveirq(void)
{
    uint16_t oldsr;
    __asm__ volatile (
        "move.w %%sr,%0\n"
        "ori.w  #0x0700,%%sr\n"
        : "=d"(oldsr) : : "memory"
    );
    return oldsr;
}

// CPU割り込みを元の状態に復帰する (ネスト可能)
//  sr: tsr_saveirqが返したSR
//  戻り値: なし
static inline void tsr_restoreirq(uint16_t sr)
{
    __asm__ volatile (
        "move.w %0,%%sr\n"
        : : "d"(sr) : "memory"
    );
}

// tsr_setvectors, tsr_restorevectors, tsr_getoldvectorで扱うベクタ情報
struct tsr_vecinfo {
    int vecno;      // ベクタ番号 (0なら配列の終端)
    void *newvec;   // 変更後の処理アドレス (呼び出し側が設定する)
    void *oldvec;   // 変更前の処理アドレス (tsr_setvectorsが設定する)
};
typedef struct tsr_vecinfo tsr_vecinfo_t;

// 複数のベクタをまとめて新しいアドレスに変更し、変更前のアドレスを保存
//  vecs: ベクタ情報配列 (vecnoが0の要素で終端)
//  戻り値: なし
void tsr_setvectors(struct tsr_vecinfo *vecs);

// 複数のベクタをまとめて変更前のアドレスに復帰する
//  vecs: ベクタ情報配列 (vecnoが0の要素で終端)
//  戻り値: 0 復帰成功
//          -1 復帰できないベクタがあった
int tsr_restorevectors(struct tsr_vecinfo *vecs);

// 指定したベクタの変更前のアドレスを返す
//  vecs: ベクタ情報配列 (vecnoが0の要素で終端)
//  vecno: 検索するベクタ番号
//  戻り値: 変更前の処理アドレス。見つからない場合はNULL
void *tsr_getoldvector(struct tsr_vecinfo *vecs, int vecno);

// 通常の関数から割り込みハンドラを呼び出す
//  entry: 呼び出す割り込みハンドラのアドレス
//  戻り値: なし
static inline void tsr_interruptcall(void *entry)
{
    __asm__ volatile (
        "tst.b   0x0cbc.w\n\t"
        "beq.s   0f\n\t"
        "clr.w   %%sp@-\n"
        "0:\n\t"
        "pea.l   %%pc@(1f)\n\t"
        "move.w  %%sr,%%sp@-\n\t"
        "jmp     %0@\n"
        "1:\n"
        :
        : "a"(entry)
        : "cc", "memory"
    );
}

// DOSコールハンドラのアセンブリ言語エントリを定義する
//  _asmentry: 定義するエントリのシンボル名
//  _handler: 呼び出すC言語ハンドラ
#define TSR_DOSHANDLER(_asmentry, _handler) \
    extern void _asmentry(void); \
    __asm__ ( \
        "       .global " #_asmentry "\n" \
        #_asmentry ":\n" \
        "       move.l  %a6,%sp@-\n" \
        "       jbsr    " #_handler "\n" \
        "       addq.l  #4,%sp\n" \
        "       rts\n\n" \
    )

// 通常の関数からDOSコールハンドラを呼び出す
//  entry: 呼び出すDOSコールハンドラのアドレス
//  arg: A6レジスタに設定する値 (スタック上のDOSコール引数へのポインタ)
//  戻り値: DOSコールハンドラの戻り値 (D0レジスタの値)
static inline int tsr_doscall(void *entry, void *arg)
{
    register int res __asm__("d0");
    register void *_a0 __asm__("a0") = entry;
    register void *_a6 __asm__("a6") = arg;
    __asm__ volatile (
        "movem.l %%d2-%%d7/%%a2-%%a6,%%sp@-\n\t"
        "jbsr    %1@\n\t"
        "movem.l %%sp@+,%%d2-%%d7/%%a2-%%a6\n"
        : "=d"(res), "+a"(_a0)
        : "a"(_a6)
        : "d1", "a1", "cc", "memory"
    );
    return res;
}

// IOCSハンドラのアセンブリ言語エントリを定義する
//  _asmentry: 定義するエントリのシンボル名
//  _handler: 呼び出すC言語ハンドラ
#define TSR_IOCSHANDLER(_asmentry, _handler) \
    extern void _asmentry(void); \
    __asm__ ( \
        "       .global " #_asmentry "\n" \
        #_asmentry ":\n" \
        "       movem.l %d0-%d7/%a1-%a6,%sp@-\n" \
        "       pea.l   %sp@\n" \
        "       jbsr    " #_handler "\n" \
        "       addq.l  #4,%sp\n" \
        "       movem.l %sp@+,%d0-%d7/%a1-%a6\n" \
        "       rts\n\n" \
    )

// 通常の関数からIOCSハンドラを呼び出す
//  entry: 呼び出すIOCSハンドラのアドレス
//  regs: 呼び出し前後のレジスタ値を格納する構造体へのポインタ
//  戻り値: なし
static inline void tsr_iocscall(void *entry, struct iocs_regs *regs)
{
    register void *_a0 __asm__("a0") = entry;
    register struct iocs_regs *_a1 __asm__("a1") = regs;
    __asm__ volatile (
        "movem.l %%d2-%%d7/%%a2-%%a6,%%sp@-\n\t"
        "move.l  %%a1,%%sp@-\n\t"
        "movem.l %%a1@,%%d0-%%d7/%%a1-%%a6\n\t"
        "jbsr    %%a0@\n\t"
        "movea.l %%sp@+,%%a0\n\t"
        "movem.l %%d0-%%d7/%%a1-%%a6,%%a0@\n\t"
        "movem.l %%sp@+,%%d2-%%d7/%%a2-%%a6\n\t"
        : "+a"(_a0), "+a"(_a1)
        :
        : "d0", "d1", "cc", "memory"
    );
}

__END_DECLS

#endif /* _TSR_H_ */
