# libtsr サンプル

このディレクトリにはlibtsrの使用例を収録しています。
libtsrのAPI仕様は [README-libtsr.md](../README-libtsr.md) を参照してください。

## ビルド

リポジトリのトップディレクトリで `make` を実行すると、ライブラリとヘッダの
インストールに続いてサンプルもビルドされます。

```sh
make
```

ライブラリとヘッダをインストール済みで、サンプルだけをビルドする場合は
トップディレクトリから `make -C sample` を実行します。

## サンプルの内容

### minidrv

登録時に指定した文字列をREAD要求で返す、最小構成のキャラクタデバイスです。
文字列を省略すると `Hello X68000 libtsr\r\n` を返します。`-q` を指定すると
登録メッセージを抑制します。デバイス名はそれぞれ `MINI1`、`MINI2`、`MINI3` です。

- `minidrv1.sys` はCONFIG.SYS専用です。INIT要求からパラメータを取得して常駐します。
- `minidrv2.x` はCONFIG.SYSと直接実行の両方に対応し、直接実行した場合は
  `-r` で常駐解除できます。登録処理と常駐部を1ファイルに収めています。
- `minidrv3.x` の機能は `minidrv2.x` と同じですが、`minidrv3main.c` に
  非常駐部、`minidrv3tsr.c` に常駐部を分けています。

```text
DEVICE=\path\minidrv1.sys [-q] [text]
DEVICE=\path\minidrv2.x [-q] [text]
DEVICE=\path\minidrv3.x [-q] [text]

minidrv2.x [-q] [text]
minidrv2.x -r
minidrv3.x [-q] [text]
minidrv3.x -r
```

`TSR_DEVICE`、`TSR_HEAP`、`TSR_NO_ENVIRON`、INITパラメータの解析、
キャラクタデバイスのREAD要求、`tsr_keepdev` と `tsr_freedev` の基本を確認できます。

### multidrv

1つの実行ファイルで `MDEV0`、`MDEV1`、`MDEV2` の3つのキャラクタデバイスを
まとめて登録します。各デバイスは共通のリクエストハンドラを使用しますが、
デバイスヘッダに保存した固有データに応じて異なる文字列を返します。

```text
multidrv.x       # 3つのデバイスを登録
multidrv.x -r    # 3つのデバイスをまとめて解除
```

`TSR_DEVICE_FIRST`、`TSR_DEVICE_NEXT`、`TSR_DEVICE_TAIL` による複数デバイス列と、
`tsr_setdevdata`、`tsr_getdevdata` によるデバイスごとの状態管理を確認できます。

### localdrv

通常のローカルブロックデバイスとして、1台以上の可変容量RAMディスクを接続します。
引数には各ユニットの容量をKiB単位で指定します。再実行すると割り当てられた
ドライブと容量を表示します。直接実行専用であり、CONFIG.SYSからのINIT要求には
エラーを返します。

```text
localdrv.x size_kib [size_kib ...]  # RAMディスクを接続
localdrv.x                          # 接続状態を表示
localdrv.x -r                       # ドライブを切断して常駐解除
```

BPBの作成、`tsr_bpb2dpb`、`tsr_formatdrive`、`tsr_attachdrive`、
`tsr_detachdrive` を使ったローカルブロックデバイスの一連の処理を確認できます。

### remotedrv

ファイルを持たない、空の読み取り専用リモートドライブを接続します。ユニット数は
省略すると1台、指定する場合は1～16台です。ディレクトリ検索、ファイル操作、
容量取得などのリモートドライブ要求に、空または読み取り専用として応答します。

```text
remotedrv.x [units]  # リモートドライブを接続
remotedrv.x          # 接続状態を表示
remotedrv.x -r       # ドライブを切断して常駐解除
```

`_DEV_REMOTE` を持つブロックデバイスの定義、リモートドライブ要求の処理、
`tsr_attachdrive` と `tsr_detachdrive` による複数ユニットの管理を確認できます。

### vecthook

DOS `_GETS`、IOCS `_B_KEYINP`、キー入力割り込みベクタをまとめてフックし、
入力文字数、呼び出し回数、最後のIOCSキーコードを常駐データへ記録します。
フック内では画面出力を行わず、再実行した非常駐側から集計を参照します。

- `vecthook1.x` は登録処理と常駐部を1ファイルに収めた入門例です。
- `vecthook2.x` は同じ機能を非常駐側の `vecthook2main.c` と常駐側の
  `vecthook2tsr.c` に分け、`m68k-xelf-tsrgen` で常駐オブジェクトを生成します。

```text
vecthook1.x       # 未常駐なら常駐、常駐済みなら集計を表示
vecthook1.x -s    # 集計を表示
vecthook1.x -c    # 集計をクリア
vecthook1.x -r    # ベクタを復帰して常駐解除

vecthook2.x       # 未常駐なら常駐、常駐済みなら集計を表示
vecthook2.x -s    # 集計を表示
vecthook2.x -c    # 集計をクリア
vecthook2.x -r    # ベクタを復帰して常駐解除
```

`TSR_PROCESS`、`TSR_DOSHANDLER`、`TSR_IOCSHANDLER`、`tsr_setvectors`、
`tsr_restorevectors`、旧ハンドラの呼び出し、割り込み禁止中の共有データ操作を
確認できます。
