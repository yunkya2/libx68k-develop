# elf2x68k ライブラリ開発用リポジトリ

## 概要
これは [elf2x68k](https://github.com/yunkya2/elf2x68k) 用ライブラリの開発と動作確認を行うためのリポジトリです。

本リポジトリで実装されているAPIや仕様は変更される場合があります。

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

`sample/`には、本リポジトリのlibtsrの使用例を収録しています。

各サンプルの用途、実行方法、常駐解除方法は[sample/README.md](sample/README.md) を参照してください。


## ビルド方法
[elf2x68k](https://github.com/yunkya2/elf2x68k) がインストールされている環境で、次のコマンドを実行します。

```sh
make
```

`make`はlibtsr、sampleの順にビルドします。
libtsrの公開ヘッダとライブラリは `include/`および`lib/`、Human68k用のサンプル実行ファイル（`.x`）は `sample/`に生成されます。

生成物を削除するには次を実行します。

```sh
make clean
```
