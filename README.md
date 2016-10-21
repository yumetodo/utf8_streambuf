# utf8_streambuf
VSでもUTF-8な文字列を入出力したい

# VSのstd::wcoutの挙動

まずそもそもVSの``std::wcout``の挙動を整理します。

```cpp
std::wcout.imbue(std::locale(""));
std::wcout
    << L"𠮷野家" << std::endl
    << L"sekai" << std::endl;
```

## コマンドプロンプトの挙動

### cmd.exe chcp 932の場合

**このコードはなにも出力しません**。
この場合コマンドプロンプトの文字コードがcp932なのでUTF16->cp932の変換をコマンドプロンプトが行います。が、一度でも変換できない文字(ex. ``𠮷``)に当たると以降一切の変換を行いません。

### cmd.exe chcp 65001の場合
この場合UnicodeなんだからUTF16->UTF-8の変換をコマンドプロンプトがやってくれることを期待したいところですが、してくれるけど表示するフォントがない、という状況になります。
![image](https://cloud.githubusercontent.com/assets/10869046/19582942/add6b258-9773-11e6-9ad8-1b6b9acf5344.png)
フォントの変更にはレジストリの変更が必要らしいのですが、私は一回もうまくやれた試しがありません。
そもそもユーザーにそんな手間を要求するのは本末転倒です。

ConEmuからcmd.exeを呼んで見ましたが

![image](https://cloud.githubusercontent.com/assets/10869046/19583051/6575211a-9774-11e6-88cb-e94a34fac055.png)

フォントを認識してくれないようです。

## nyagosの挙動

Windowsで使える他のコンソールとしてnyagosがあります。これをConEmuから使ってみます

### chcp 932のとき
![image](https://cloud.githubusercontent.com/assets/10869046/19583083/b3dbd506-9774-11e6-81f8-d64b00af1706.png)

この場合コマンドプロンプトの文字コードがcp932なのでUTF16->cp932の変換をコマンドプロンプトが行います。変換できない文字はスキップして変換をする仕様のようです。

### chcp 65001のとき

![image](https://cloud.githubusercontent.com/assets/10869046/19583139/2d51d732-9775-11e6-9ae2-4b9de06c7d66.png)
やはりUTF16->UTF-8の変換はされてもフォントがうまく認識してくれないようです。

# このライブラリは何をするか

UTF8->UTF16への変換をします。

Windows環境ではstd::coutが直接ロケール依存の文字コードなので、Linuxとコードの共通化ができません。
そこでWindows環境、といってもmingwのstd::wcoutがぶっ壊れているのはさんざん述べたとおりなので、実質VS環境のみですが、std::coutにはUTF-8の文字列を流し込むことにして、それをUTF16に変換してstd::wcoutにバイパスします。ただそれだけです。
単純な変換しかしないので上記の問題はそっくりそのまま残ります。が、コードの共通化のために有用と考えます。

# このライブラリを使うときのフロー

## Linux環境

端末の文字コードはUTF-8と仮定します。EUC-JPの可能性は低いと考えます。
この場合``std::cout``はUTF-8な文字列を流せばいいのでとくにすることはありません。

## msys2 mingw環境

端末にbash.exe+mintty.exeなどのcmd.exeに依存しない何かを使うことを前提にします。
この文字コードはUTF-8と仮定します。
この場合``std::cout``はUTF-8な文字列を流せばいいのでとくにすることはありません。

## Visual Studio環境

cmd.exe上で動かすことを仮定します。先の過程より、chcp932な環境に出力することになります。
std::cout -> u8ostreambuf -> std::wcout -> chcp932への変換(cmd.exe) -> 出力
という流れになります。

つまり

```cpp
#ifdef _MSC_VER
std::wcout.imbue(std::locale(""));
u8ostreambuf buf1{ std::wcout.rdbuf() };
u8istreambuf buf2{ std::wcin.rdbuf() };
auto tmp1 = std::cout.rdbuf(&buf1);
auto tmp2 = std::cin.rdbuf(&buf2);
#endif
```

のようにどこかに書いておいて、

```cpp
std::cout << u8"ありきたりな世界" << std::endl;
```

のように使えます。

