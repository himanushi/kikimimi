# WebSockets の WEBSOCKETS_MAX_DATA_SIZE(15KB)は Realtime API の音声 delta フレームより
# 小さく、超過フレームで即切断(close 1009)される。#ifndef ガードがなく build_flags では
# 上書きできないため、libdeps 展開後のヘッダを 256KB に書き換える。
# 大きな受信 payload の malloc は PSRAM 側(SPIRAM_USE_MALLOC)に逃げるので内部 RAM は枯渇しない
Import("env")
import pathlib

header = pathlib.Path(env.subst("$PROJECT_LIBDEPS_DIR"), env.subst("$PIOENV"),
                      "WebSockets", "src", "WebSockets.h")
if header.exists():
    text = header.read_text()
    patched = text.replace("(15 * 1024)", "(256 * 1024)")
    if patched != text:
        header.write_text(patched)
        print("patched: WEBSOCKETS_MAX_DATA_SIZE 15KB -> 256KB")
