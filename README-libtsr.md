# libtsr APIリファレンス

## 概要

libtsrは、Human68k上でメモリ常駐プログラムを実装するためのライブラリです。

libtsrは2種類の常駐方式を扱います。

- デバイスドライバ: デバイスヘッダを備えた、Human68kからキャラクタまたはブロックデバイスとして扱われる形式の常駐プログラムです
- 常駐プロセス: デバイスヘッダを持たない、通常のプロセスが常駐終了する形式の常駐プログラムです

## デバイスドライバ定義・API

### デバイスヘッダ

```c
struct dos_devheader {
    struct dos_devheader *next;
    uint16_t attr;
    void *strategy;
    void *interrupt;
    char name[8];
};

struct tsr_devheader {
    struct dos_devheader header;
    void *data;
};
typedef struct tsr_devheader *tsr_device_t;

struct tsr_devheader_first {
    struct tsr_devheader devheader;
    uint32_t signature;
    void *memblock;
    struct dos_devheader *tail;
};
```

`struct dos_devheader` はHuman68k仕様の22バイトのデバイスヘッダです。

libtsrが扱うデバイスヘッダは、これにドライバ固有データ用の `data` を追加した `struct tsr_devheader` で、APIからはこのヘッダへのポインタを `tsr_device_t` 型の値として扱います。単一デバイスおよび複数デバイス構成の先頭ヘッダは `struct tsr_devheader_first` として配置され、`TSR_SIGNATURE`、常駐メモリブロック、複数デバイス構成の末尾を保持します。


### デバイス属性

デバイスヘッダの `attr` フィールドは、Human68kの仕様に従い、デバイスの種類や機能を示す属性値を持ちます。libtsrでは、次の定数で定義されます。

| 属性マクロ | 値 | 意味 |
|---|---:|---|
| `_DEV_BDEV` | `0x0000` | ブロックデバイス |
| `_DEV_CDEV` | `0x8000` | キャラクタデバイス |
| `_DEV_STDIN` | `0x0001` | 標準入力 |
| `_DEV_STDOUT` | `0x0002` | 標準出力 |
| `_DEV_NUL` | `0x0004` | NULデバイス |
| `_DEV_CLOCK` | `0x0008` | CLOCKデバイス |
| `_DEV_RAW` | `0x0020` | RAW MODE |
| `_DEV_SPIOCTRL` | `0x0040` | 特殊IOCTRL対応 |
| `_DEV_REMOTE` | `0x2000` | リモートドライブ |
| `_DEV_IOCTRL` | `0x4000` | IOCTRL対応 |

### デバイスの定義

#### 単一デバイス

* **TSR_DEVICE**(*_attr*, *_name*, *_interrupt*);
    * 単一デバイスのヘッダとストラテジ/割り込みルーチンを定義します
    * *_attr* はデバイス属性、*_name* は8文字のデバイス名、*_interrupt* は デバイスリクエスト処理関数を指定します

1つのデバイスのみを扱うデバイスドライバは、そのデバイスヘッダを `TSR_DEVICE` で定義します。リクエスト処理関数の *rh* にはリクエストヘッダ、*dev* にはその処理関数を指定したデバイスのヘッダが渡されます。

#### 複数デバイス

* **TSR_DEVICE_FIRST**(*_attr*, *_name*, *_interrupt*, *_next*, *_tail*);
    * 複数デバイスで構成するデバイスドライバの先頭デバイスを定義します
    * *_next* は次のデバイスの識別子、*_tail* は末尾のデバイスの識別子を指定します
* **TSR_DEVICE_NEXT**(*_attr*, *_name*, *_interrupt*, *_id*, *_next*);
    * 複数デバイスで構成するデバイスドライバの中間デバイスを定義します
    * *_id* はこのデバイスの識別子、*_next* は次のデバイスの識別子を指定します
* **TSR_DEVICE_TAIL**(*_attr*, *_name*, *_interrupt*, *_id*);
    * 複数デバイスで構成するデバイスドライバの末尾デバイスを定義します
    * *_id* はこのデバイスの識別子を指定します

複数のデバイスを持つデバイスドライバは、先頭デバイスを `TSR_DEVICE_FIRST` 、中間デバイスを `TSR_DEVICE_NEXT` 、末尾デバイスを `TSR_DEVICE_TAIL` で定義します。

例えば、`DEV0`, `DEV1`, `DEV2` の3つのキャラクタデバイスを持つデバイスドライバは、次のように定義します。

```c
TSR_DEVICE_FIRST(_DEV_CDEV, "DEV0    ", handler, 1, 2);
TSR_DEVICE_NEXT (_DEV_CDEV, "DEV1    ", handler, 1, 2);
TSR_DEVICE_TAIL (_DEV_CDEV, "DEV2    ", handler, 2);
```

#### デバイスの参照

* **TSR_THISDEV**
    * 現在のデバイスドライバの先頭デバイスを取得します
* **TSR_THISDEV_ID**(*_id*)
    * 現在のデバイスドライバの指定した識別子のデバイスを取得します

ソースコード中の自分自身のデバイスを参照する場合は、`TSR_THISDEV` または `TSR_THISDEV_ID` を使用します。複数デバイス構成のデバイスドライバでは、識別子を指定して特定のデバイスを取得できます。`tsr_device_t` 型の値としてAPIへ渡せます。

### リクエストヘッダとリクエスト処理関数

`TSR_DEVICE*` で定義したデバイスのリクエスト処理関数は、Human68kの仕様に従い、`struct dos_req_header` で渡されるリクエストヘッダを参照して処理を行います。

```c
struct dos_req_header {
    uint8_t magic;
    uint8_t unit;
    uint8_t command;
    uint8_t errl;
    uint8_t errh;
    uint8_t reserved[8];
    uint8_t attr;
    void *addr;
    uint32_t status;
    void *fcb;
};
```

リクエスト処理関数は以下のプロトタイプで定義します。

```c
int handler(struct dos_req_header *rh, tsr_device_t dev);
```

*rh* はリクエストヘッダへのポインタ、*dev* はこの処理関数を指定したデバイスヘッダへのポインタです。リクエスト処理関数の戻り値は `errl` と `errh` に格納されます。


### デバイスの検索・常駐・解除

* tsr_device_t **tsr_finddev**(const char \**name*);
    * `TSR_SIGNATURE` を持ち、*name* に一致するlibtsrの先頭デバイスを返します
    * *name* が NULL、またはデバイスが見つからない場合は NULL を返します
* void **tsr_setdevdata**(tsr_device_t *dev*, void \**data*);
    * *dev* のデバイス固有データを *data* に設定します
* void \***tsr_getdevdata**(tsr_device_t *dev*);
    * *dev* のデバイス固有データを返します
* void **tsr_keepdev**(void \**tsrend*, int *code*);
    * 自分のヘッダをHuman68kのデバイスチェイン末尾に連結し、終了コード *code* で現在のプロセスを常駐させます
    * *tsrend* は常駐範囲の終了アドレスを指定します。NULL の場合は `tsr_getresidentend()` が返す既定値を使用します
    * この関数は戻りません
* void \***tsr_getresidentend**(void);
    * 非常駐部分離型なら常駐部の末尾、それ以外は `_end` が示すプログラムの静的領域末尾を返します
    * CONFIG.SYS で登録したデバイスの常駐範囲を指定する際は、この関数の戻り値をリクエストヘッダの `addr` フィールドに設定します
    * 通常のヒープ、スタックは常駐範囲に含まれません。常駐部にこれらを含める場合は、後述の `TSR_HEAP` や `TSR_STACK` を使用して静的に確保してください
* int **tsr_isfreeable**(tsr_device_t *dev*);
    * *dev* が`tsr_freedev`で解放可能であれば true、そうでなければ false を返します
    * CONFIG.SYS で登録されたデバイスは登録解除できないため、false を返します
* int **tsr_freedev**(tsr_device_t *dev*);
    * *dev* のデバイス列をデバイスチェインから外し、常駐メモリを解放します
    * 成功時は0、対象が存在しないか解除できない場合は負値を返します

デバイスヘッダはスーパーバイザ領域に存在する場合があるため、取得したハンドルの内容は
直接参照せず、libtsrのAPIへ渡して使用します。自分の先頭デバイスには `TSR_THISDEV`、
登録済みデバイスには `tsr_finddev` の戻り値を渡します。ヘッダへのアクセスはAPI内部で
スーパーバイザモードに切り替えて行われます。

### 初期化パラメータ変換

* int **tsr_parseinitparam**(void \**param*, char \*\**argv*, int *maxargv*);
    * `CONFIG.SYS` で組み込んだデバイスドライバのデバイス初期化コマンド `0x00` に渡されるパラメータを C言語の *argc*,*argv* 形式へ変換します
    * *param* にはリクエストヘッダの `status` フィールドの値を指定します
    * *argv* には *maxargv* 要素以上の配列を渡します。*param* 内へのポインタを最大 *maxargv* - 1個格納し、末尾を NULL で終端します。文字列は複製されません
    * 格納した引数の数を返します。


## 常駐プロセス定義・API

```c
struct tsr_process {
    uint32_t signature;
    char name[16];
    void *memblock;
    void *data;
};
typedef struct tsr_process *tsr_process_t;
```

`struct tsr_process` は、常駐プロセスのヘッダです。`signature` は `TSR_SIGNATURE`、`name` は最大16バイトの常駐プロセス名、`memblock` は常駐メモリブロックの先頭アドレス、`data` は常駐プロセス固有データへのポインタを保持します。
APIからはこのヘッダへのポインタを `tsr_process_t` 型の値として扱います。

### 常駐プロセスの定義

* **TSR_PROCESS**(*_name*);
    * `.header` セクションに非デバイスドライバ型の常駐プロセスヘッダを1個定義します
    * *_name* は最大16バイトの常駐プロセス名を指定します

### 常駐プロセスの参照

* **TSR_THISPROC**
    * 現在の常駐プロセスのヘッダを `tsr_process_t` として取得します

### 常駐プロセスの検索・常駐・解除

* tsr_process_t **tsr_findproc**(const char \**name*);
    * 常駐メモリブロックを走査し、`TSR_SIGNATURE` を持ち、*name* が最大16バイトで一致する常駐プロセスを返します
    * 見つからない場合は NULL を返します
* void **tsr_setprocdata**(tsr_process_t *proc*, void \**data*);
    * *proc* の常駐プロセス固有データを *data* に設定します
* void \***tsr_getprocdata**(tsr_process_t *proc*);
    * *proc* の常駐プロセス固有データを返します
* void **tsr_keepproc**(void \**tsrend*, int *code*);
    * 自分のヘッダに現在のメモリブロックを記録し、終了コード *code* でプロセスを常駐させます
    * *tsrend* は常駐範囲の終了アドレスを指定します。NULL の場合は `tsr_getresidentend()` が返す既定値を使用します
    * この関数は戻りません
* int **tsr_freeproc**(tsr_process_t *proc*);
    * *proc* の常駐メモリを解放します
    * 成功時は0、*proc* が NULL またはメモリブロックを持たない場合は -1 を返します


## ブロックデバイスサポートAPI

### エラーコード

ブロックデバイスサポートAPIは以下のエラーコードを返します。

```c
#define TSR_ERR_SECTSIZE   (-1)
#define TSR_ERR_CLUSTERS   (-2)
#define TSR_ERR_INUSE      (-3)
#define TSR_ERR_NOSPACE    (-4)
#define TSR_ERR_NOTFOUND   (-5)
#define TSR_ERR_NODEV      (-6)
#define TSR_ERR_BUSY       (-7)
#define TSR_ERR_NOBUF      (-8)
```

### DOS BPB定義

```c
struct dos_bpb {
    uint16_t sectbytes;
    uint8_t sectclust;
    uint8_t fatnum;
    uint16_t resvsects;
    uint16_t rootent;
    uint16_t sects;
    uint8_t mediabyte;
    uint8_t fatsects;
    uint32_t sectslong;
};
```

`fatnum` のbit 7はMS-DOS形式FAT、下位7ビットはFAT数を表します。`sects == 0` なら
`sectslong` が全セクタ数に使われます。`struct dos_dpb` はHuman68kのローカルドライブ用
Drive Parameter Block、`struct dos_dpb_remote` は共通部分だけを持つリモートドライブ用定義、
`struct dos_curdir` はHuman68kのカレントディレクトリテーブル要素です。

### API

* int **tsr_bpb2dpb**(struct dos_bpb \**bpb*, struct dos_dpb \**dpb*);
    * *bpb* から *dpb* を構築します
    * 成功時は0、セクタサイズが大きすぎる場合は `TSR_ERR_SECTSIZE`、クラスタ数が多すぎる場合は `TSR_ERR_CLUSTERS` を返します
* int **tsr_attachdrive**(int *drive*, int *units*, struct dos_dpb \*\**dpbs*, tsr_device_t *dev*);
    * *units* 個のDPBをHuman68kのDPBチェインとカレントディレクトリテーブルに接続します
    * *drive* が -1 の場合は空きドライブを先頭から自動選択し、0以上の場合は0をA:、1をB:とする指定番号から接続します
    * *dpbs* は *units* 個の `struct dos_dpb *` を持つ配列、*dev* は対象デバイスを指定します
    * 成功時は0、指定ドライブが使用中の場合は `TSR_ERR_INUSE`、空きドライブが不足する場合は `TSR_ERR_NOSPACE` を返します
* int **tsr_getlogicaldrive**(int *realdrive*);
    * DPBに格納された実ドライブ番号 *realdrive* をユーザー向けの論理ドライブ番号へ変換します
    * 戻り値は0がAドライブ、1がBドライブに対応し、対応するドライブがない場合は `TSR_ERR_NOTFOUND` を返します
* int **tsr_formatdrive**(struct dos_dpb \**dpb*, tsr_preparesect_t \**preparesect*, tsr_commitsect_t \**commitsect*, void \**arg*);
    * *dpb* の内容に従い、FATとルートディレクトリを論理フォーマットします
    * *preparesect* 、*commitsect* には、セクタの読み書きを行う以下の型のコールバック関数へのポインタを指定します
        * typedef void \***tsr_preparesect_t**(void \**arg*, int *sect*);
            * *sect* で指定したセクタの書き込み可能なバッファを用意するコールバック型
            * バッファを用意できない場合は NULL を返します
        * typedef int **tsr_commitsect_t**(void \**arg*, int *sect*, void \**buf*);
            * *buf* の内容を *sect* で指定したセクタへ反映するコールバック型
            * RAMディスクのように `tsr_preparesect` が返したバッファがデータ領域そのものである場合は、NULL を指定できます
            * 成功時は0以上、失敗時は負値を返します
    * *arg* は両コールバックの第1引数へそのまま渡されます
    * 成功時は0、バッファを取得できない場合は `TSR_ERR_NOBUF`、書き戻しに失敗した場合は *commitsect* が返したエラー値を返します
* int **tsr_detachdrive**(tsr_device_t *dev*);
    * *dev* のデバイスに接続された全ローカルドライブを切断します
    * 成功時は0、*dev* が NULL の場合は `TSR_ERR_NODEV`、対象デバイスが使用中の場合は `TSR_ERR_BUSY` を返します


## ヒープ・スタック管理API

* **TSR_HEAP**(*_size*);
    * *_size* バイトの固定ヒープ領域を定義します
* **TSR_STACK**(*_size*);
    * *_size* バイトの固定スタック領域を定義します
* **TSR_NO_ENVIRON**();
    * プログラムの起動処理ではプロセスに設定されている環境変数をenviron変数から参照できるようにヒープ領域にコピーする処理を行っていますが、この処理を無効化します。
    * ユーザが大量の環境変数を設定しているとヒープ領域の消費量が増えるため、`TSR_NO_ENVIRON` を定義することでヒープ領域の消費量を抑えることができます。
* **TSR_KEEP_SOCKET**();
    * 常駐時の終了処理によってsocketがcloseされないようにします
* **TSR_KEEP_PTHREAD**();
    * 常駐時の終了処理によってthreadがcloseされないようにします
* size_t **tsr_heapfree**(void);
    * 現在のヒープ領域の残りバイト数を返します

ヒープを扱う常駐プログラムは `TSR_HEAP` を必ず定義します。常駐後に必要なヒープ容量を `_size` に指定します。この固定ヒープは静的領域内に配置されるため、分離型と非分離型の
どちらでも常駐範囲に含まれます。通常ヒープを拡張して得た領域を常駐後に参照してはなりません。

`TSR_NO_ENVIRON` は固定heapの消費を予測しやすくするために使います。`TSR_KEEP_SOCKET` と
`TSR_KEEP_PTHREAD` は、常駐後もそれぞれsocketやthreadを利用するプログラムで定義します。


## DOS/IOCSベクタ・割り込み管理API

### ベクタ管理

```c
struct tsr_vecinfo {
    int vecno;
    void *newvec;
    void *oldvec;
};
typedef struct tsr_vecinfo tsr_vecinfo_t;
```

`tsr_setvectors`、`tsr_restorevectors`、`tsr_getoldvector` で扱うベクタ情報です。
`vecno == 0` の要素が配列終端となります。
`newvec` は変更後の処理アドレスとして呼び出し側が設定し、`oldvec` は変更前の
処理アドレスとして `tsr_setvectors` が設定します。

* void **tsr_setvectors**(struct tsr_vecinfo \**vecs*);
    * *vecs* の各ベクタを `newvec` へ変更し、元のアドレスを `oldvec` へ保存します
* int **tsr_restorevectors**(struct tsr_vecinfo \**vecs*);
    * *vecs* の全ベクタが現在も `newvec` を指すことを確認してから `oldvec` へ復帰します
    * 成功時は0、一つでも復帰できないベクタがある場合は何も復帰せず -1 を返します
* void \***tsr_getoldvector**(struct tsr_vecinfo \**vecs*, int *vecno*);
    * *vecs* から *vecno* に一致する要素の変更前の処理アドレス `oldvec` を返します
    * 見つからない場合は NULL を返します

各APIの *vecs* は `vecno` が0の要素で終端するベクタ情報配列です。


### 旧ハンドラの呼び出し

#### CPU割り込み

`tsr_setvectors` で登録する割り込みハンドラは、`__attribute__((interrupt))` で定義する必要があります。割り込みハンドラへのポインタをC関数から呼び出す場合は以下のAPIを使用します。

* void **tsr_interruptcall**(void \**entry*);
    * 通常の関数から、*entry* で指定した `rte` で戻る割り込みハンドラを呼び出す
    * スーパーバイザモードで呼び出す必要があります

#### DOSコール

DOSコール処理を行うハンドラは通常のCの関数としては呼び出せないため、DOSコールベクタからC関数を呼び出すためのアセンブラ入口を生成するAPIを提供します。

* **TSR_DOSHANDLER**(*_asmentry*, *_handler*);
    * *_asmentry* というアセンブラ入口を生成し、Human68kが `a6` で渡したDOS引数ポインタをC関数 *_handler* へ渡します
    * *_handler* は `int handler(void *arg)` 形式とし、戻り値は `d0` でコール元へ返ります
* int **tsr_doscall**(void \**entry*, void \**arg*);
    * `a6 = arg` で *entry* のDOSコールハンドラを呼び、その `d0` の値を返します

#### IOCSコール

IOCSコール処理を行うハンドラは通常のCの関数としては呼び出せないため、IOCSコールベクタからC関数を呼び出すためのアセンブラ入口を生成するAPIを提供します。

* **TSR_IOCSHANDLER**(*_asmentry*, *_handler*);
    * *_asmentry* というアセンブラ入口を生成し、入口時の `d0-d7/a1-a6` を `struct iocs_regs` と同じ順序でC関数 *_handler* へ渡します
    * *_handler* は `void handler(struct iocs_regs *regs)` 形式とし、*regs* の変更は復帰時のレジスタに反映されます
* void **tsr_iocscall**(void \**entry*, struct iocs_regs \**regs*);
    * *regs* のレジスタ値で *entry* のIOCSハンドラを呼び、復帰時の値を *regs* へ書き戻します

IOCSのスクラッチレジスタ `a0` は両APIの入出力に含まれません。


### スーパーバイザモードと割り込み制御

* int **tsr_super**(int *ssp*);
    * *ssp* が0の場合はスーパーバイザモードへ切り替え、切り替え前のSSPを返します
    * ユーザモードへ復帰するときは、保存した非0のSSPを *ssp* に渡して再度呼び出します
    * すでにスーパーバイザモードだった場合は0を返します

```c
int ssp = tsr_super(0);
/* スーパーバイザモードでの処理 */
tsr_super(ssp);
```

すでにスーパーバイザモードなら `_iocs_b_super(0)` の `-1` を `tsr_super` が `0` に
変換するため、上の形でネスト可能です。libtsrの公開APIは必要な場合に内部でモードを
切り替えるため、通常は呼び出し側での切り替えは不要です。

* uint16_t **tsr_saveirq**(void);
    * CPU割り込みを禁止し、禁止前のSRを返します
* void **tsr_restoreirq**(uint16_t *sr*);
    * `tsr_saveirq` が返した *sr* を使ってCPU割り込みを元の状態に復帰します

どちらもスーパーバイザモード限定です。保存したSRを対にして使う場合はネスト可能です。


## 常駐部と非常駐部の分離

常駐後に不要な初期化コードを分離する場合は `m68k-xelf-tsrgen` を使います。

1. 常駐させる関数とデータを専用のオブジェクトにまとめる。
2. `m68k-xelf-tsrgen -o resident.o resident-src.o` を実行します。
3. `resident.o` を非常駐部の `main` とリンクします。
4. `tsr_keepdev(NULL, code)` または `tsr_keepproc(NULL, code)` で常駐します。

常駐部が依存する追加ライブラリは、次のいずれかの形式でpartial linkに追加できます。

```sh
m68k-xelf-tsrgen -o resident.o resident-src.o --library path/to/libfoo.a
m68k-xelf-tsrgen -o resident.o resident-src.o -L path/to/lib -lfoo
```

`--library`、`-L`、`-l` はそれぞれ複数指定できます。アーカイブのパスを通常の位置引数として
渡す従来の形式も、リンカ引数としてそのまま利用できます。

partial link後にstrongな未解決シンボルが残っている場合、ツールはエラー終了します。
これにより、常駐後に呼ばれるコードが非常駐部の関数や変数を参照することを防ぎます。
最終リンク時にリンカスクリプトが定義するシンボルとweak未解決シンボルは検査対象外となります。

CONFIG.SYSのINITエントリからのみ呼び出す非常駐部の処理など、意図的に最終リンクまで
未解決のまま残すシンボルは `-u` または `--allow-undefined` で個別に許可します。

```sh
m68k-xelf-tsrgen -o resident.o resident-src.o \
    -u initialize_driver
```

複数のシンボルを許可する場合はオプションを繰り返します。

常駐後に呼ばれるコードから非常駐部の関数やデータを参照してはなりません。


## 利用例

### デバイスドライバ

```c
#include <x68k/tsr.h>

#define DRIVER_NAME "EXAMPLE "
int request_handler(struct dos_req_header *rh, tsr_device_t dev);

TSR_DEVICE(_DEV_CDEV, DRIVER_NAME, request_handler);
TSR_HEAP(1024);
TSR_NO_ENVIRON();

int request_handler(struct dos_req_header *rh, tsr_device_t dev)
{
    (void)dev;
    switch (rh->command) {
    case 0x00:
        rh->addr = tsr_getresidentend();
        return 0;
    default:
        return 0x7003;
    }
}

int main(void)
{
    tsr_device_t dev = tsr_finddev(DRIVER_NAME);
    if (dev == NULL)
        tsr_keepdev(NULL, 0);
    return tsr_freedev(dev) < 0;
}
```

### ベクタフックを持つTSR

```c
#include <x68k/tsr.h>

#define TSR_NAME "EXAMPLE TSR"
TSR_PROCESS(TSR_NAME);
TSR_HEAP(1024);
TSR_NO_ENVIRON();

static void handler(void) __attribute__((interrupt));
static struct hook_data {
    tsr_vecinfo_t vecs[2];
} hook_data = {
    .vecs = {
        { .vecno = 0x4c, .newvec = handler },
        { 0 }
    }
};

static void handler(void)
{
    tsr_interruptcall(hook_data.vecs[0].oldvec);
    /* 独自の処理 */
}

int main(void)
{
    tsr_process_t proc = tsr_findproc(TSR_NAME);
    if (proc == NULL) {
        tsr_setprocdata(TSR_THISPROC, &hook_data);
        tsr_setvectors(hook_data.vecs);
        tsr_keepproc(NULL, 0);
    }
    struct hook_data *resident = tsr_getprocdata(proc);
    if (tsr_restorevectors(resident->vecs) < 0)
        return 1;
    return tsr_freeproc(proc) < 0;
}
```

常駐部と非常駐部を分ける実用例では、`tsr_setprocdata` で常駐部の管理構造体への
ポインタを保存し、再実行時に `tsr_getprocdata` で取得します。
