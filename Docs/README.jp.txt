Palm OS(R) Emulator(POSE) <CLIE>ROMイメージ対応バージョンについて

[全般]
・本プログラムはフリー・ソフトウェアであり、いかなる保証も行ないません。
  Free Software Foundation が公表した GNU GENERAL PUBLIC LICENSE の
  「バージョン２」或いはそれ以降の各バージョンの中からいずれかを選択し、
  そのバージョンが定める条項に従って本プログラムを再頒布または変更する
  ことができます。
  再頒布にあたっては本プログラムに GNU GENERAL PUBLIC LICENSE を同梱して
  ください。

・本プログラムはPalm OS Emulator 3.4 をベースにしています。

・本プログラムはPEG-NR70シリーズ専用です。

・初めて立ち上げる場合は、Windowsシステムフォルダにある
　Palm OS Emulator.ini ファイルを消去してから立ち上げてください。

・推奨環境
	CPU: Pentium III 500MHz 以上
	Memory: 128MB 以上
	OS: Windows(R) 2000, XP

・CLIEをエミュレーションする場合は、New Session画面にて選択する際に
  ROM FileとしてCLIEのROMイメージをお選びください。

・本プログラムは、CLIEのハードウェアを完全にエミュレーションしている
  わけではありません。
  プログラムの動作確認としては、最終的には実機を御使用ください。

・本プログラムは"ROM Transfer"をサポートしていません。

[機能制限]
本プログラムは以下の機能制限があります。
・Backボタン／アプリケーションボタン／スクロールボタンを長押しする
  状態をエミュレートできません。

・電源ボタンのON/OFFが効きません。

・PEG-NR70のCDからインストールしたNavin' You Pocketを起動するとずっと
  実行中になり、他のボタンを押しても反応せず、応答なしとなります。

・初期設定のApplicationを起動するとPalm OS Emulatorのダイアログが
  出てきます。

・PEG-NR70のCDからCLIE Mailをインストールする際にエラーダイアログが
  でます。

[Memory Stick]
・エミュレーターの左側に在る in/out switchを押す事により、Memory Stickの
  in/outを切り換える事が可能です。

・Memory Stickの仮想フォルダーはエミュレーターを新規起動した際に
  Emulator.exeと同じフォルダーに以下のMSサイズ対応にて作成されます。
  Ms8:8Mbyte MS / Ms16:16Mbyte MS / Ms32:32Mbyte MS /
  Ms64:64Mbyte MS / Ms128:128Mbyte MS

・作成された MS* のフォルダーをフォルダーのプロパティにて「読み取り専用」
  にする事によって「MemoryStickの書き込み禁止」状態をエミュレートする事が
  可能です。

・上記書き込み禁止を切り換える際は、Memory Stickを抜いた状態で行ってくだ
  さい。
  挿した状態のまま切り換えた場合は正しいエミュレーションが出来ません。


[Jog Dial]
・JogEventは「Push」を一度押した際に vchrJogPressが発行され、
  再度「Push」を押した時に vchrJogReleaseが発行されます。

・押し回しのエミュレーションは「Push」を押した状態で「Up」「Down」
  を押す事により可能です。


[その他]
・エミュレーターの電源をOFFにした状態で、アプリケーションファイルのインス
  トールを実行すると、エミュレーターがハングアップすることがありますので
  ご注意ください。

・メニューの"Save Bound Emulator"の機能は、Windows95,98,98SE,ME では機能
  しません。
  Windows NT4.0, 2000Professional Editionのみ機能します。

・メニューの"Save","Save As","Save Screen"を実行した場合、セーブされる
  ファイルの拡張子".psf",".bmp"は自動的には付加されませんのでご注意くだ
  さい。

・メニューの"Settings"->"Properties"で"Enable sounds"にチェックを入れた
  状態で、予定表アプリケーションを起動しアラームをセットしても、アラーム
  がならない場合があります。

・メニューの"Install Application/Database"で、以前のログを用いてprcファイ
  ルをインストールする場合、そのPathにprcファイルが存在しなくなっている場
  合には、なにもせずにメニューが終了します。
  （Warningメッセージは出ません)

・Codewarriorとエミュレーターを接続して御使用になられた場合、ステップ実行
  やプログラムの転送がうまく動作しないことがあります。

・アドレスから画像を選択した状態でカテゴリーを変更すると、エラーダイアロ
  グが表示されてリセットすることがあります。

※最新版は以下のサイトよりダウンロードできます（会員登録が必要）。
  http://www.sony.co.jp/CLIE/dev/

※CLIE、Jog Dial、Memory Stick は、ソニー株式会社の商標です。

※Windows は米国Microsoft Corporationの登録商標です。

Copyright (c) 2000-2001 Sony Corporation