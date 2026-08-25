# elf2x68k ライブラリ開発用リポジトリ

## 概要
これは [elf2x68k](https://github.com/yunkya2/elf2x68k) 用ライブラリの開発と動作確認を行うためのリポジトリです。

本リポジトリで実装されているAPIや仕様は変更される場合があります。


## libx68k

`libx68k/` は X680x0環境のCランタイム、システムコール、起動処理を提供するライブラリです。

### DOS/IOCSコールのインライン呼び出し機能

通常、`<x68k/dos.h>`と`<x68k/iocs.h>`はDOS/IOCSコールラッパー関数のプロトタイプを宣言しますが、`__DOS_INLINE__`または`__IOCS_INLINE__`を定義してからヘッダをインクルードすると、関数が呼び出し元にインライン展開されるようになります。

`<x68k/dos_inline.h>`または`<x68k/iocs_inline.h>`を直接インクルードしても、それぞれのインライン版が有効になります。逆に`<x68k/dos_proto.h>`または`<x68k/iocs_proto.h>`をインクルードすると、従来どおりライブラリ内のラッパー関数が使われます。

#### 使用例

ソースファイル内で有効にする例を次に示します。マクロは、対応するヘッダを最初に
インクルードするより前に定義してください。

```c
#define __DOS_INLINE__
#define __IOCS_INLINE__
#include <x68k/dos.h>
#include <x68k/iocs.h>

int main(void)
{
	_dos_print("Hello from DOS\r\n");
	_iocs_b_print("Hello from IOCS\r\n");

	int key = _iocs_b_keyinp();
	_dos_putchar(key & 0xff);

	_dos_exit();
}
```

複数のソースファイルで一括して有効にする場合は、ビルドオプションで定義します。

```make
CFLAGS += -Os -D__DOS_INLINE__ -D__IOCS_INLINE__
```

インライン版は呼び出し箇所ごとに命令が展開されるため、同じコールを多数の場所から
使用するとコードサイズが増える場合があります。その場合は、対象のマクロを定義せず
ライブラリ版を使用してください。


### DOS/IOCSコールラッパー関数のCSV生成対応

DOSコールおよびIOCSコールのラッパー関数をCSVから生成する方式へ移行しました。

- DOSコール: [libx68k/libdos/doscall.csv](libx68k/libdos/doscall.csv)
- IOCSコール: [libx68k/libiocs/iocscall.csv](libx68k/libiocs/iocscall.csv)

各CSVのフォーマット仕様は [README-wrapper-csv.md](README-wrapper-csv.md) を参照してください。


### libtsr対応拡張

libtsrによる常駐プログラムやデバイスドライバ開発対応のため、リンカスクリプトや起動処理を拡張しました。

- 通常、プログラムのスタックやヒープ領域はBSS領域の後ろに配置されますが、静的に確保されたスタックやヒープ領域が存在する場合にはそれを利用するようにしました。
- 常駐プログラムで常駐部と非常駐部を分離して配置できるようにしました。
- プログラムの起動処理ではプロセスに設定されている環境変数をenviron変数から参照できるようにヒープ領域にコピーする処理を行っていますが、この処理を行わない設定を可能にしました (`TSR_NO_ENVIRON`対応)。


## libtsr

`libtsr/` は、Human68kのメモリ常駐プログラムやデバイスドライバをC言語で実装するためのライブラリです。以下のような機能を提供しています。

- デバイスドライバで必要となるデバイスヘッダの定義
- デバイスドライバのCONFIG.SYS登録とHuman68k起動後の登録の両立
- プログラムの常駐処理、常駐状態の確認、常駐解除
- ブロックデバイスドライバのドライブ接続と切断
- 常駐部と非常駐部の分離
- スタック、ヒープ領域の静的確保
- DOS/IOCS/割り込みベクタの変更と復帰
- DOS/IOCS/割り込みサービスのC言語による記述、C言語からの呼び出し

APIの詳細は [README-libtsr.md](README-libtsr.md) を参照してください。


## sample

`sample/`には、libx68kおよびlibtsrの使用例を収録しています。

各サンプルの用途、実行方法、常駐解除方法は[sample/README.md](sample/README.md) を参照してください。


## ビルド方法
[elf2x68k](https://github.com/yunkya2/elf2x68k) がインストールされている環境で、次のコマンドを実行します。

```sh
make
```

`make`はlibx68k、libtsr、sampleの順にビルドします。
公開ヘッダとライブラリ、起動用オブジェクトは `include/`および`lib/`、Human68k用のサンプル実行ファイル（`.x`）は `sample/`に生成されます。

生成物を削除するには次を実行します。

```sh
make clean
```
