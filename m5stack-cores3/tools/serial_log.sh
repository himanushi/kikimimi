#!/bin/bash
# CoreS3 のシリアル出力を logs/serial-<日時>.log に保存し続ける(デバッグ用)。
# logs/latest.log が常に最新ログを指す。切断(リセット・アップロード)しても再接続して追記する。
# 注意: このスクリプトがポートを掴んでいる間は pio upload が失敗する。書き込み前に停止すること。
# 使い方: tools/serial_log.sh [ポート]   停止: Ctrl-C または kill
set -u
cd "$(dirname "$0")/.."
PORT="${1:-$(ls /dev/cu.usbmodem* 2>/dev/null | head -1)}"
if [ -z "$PORT" ]; then
    echo "シリアルポートが見つかりません(USB 接続を確認)" >&2
    exit 1
fi
mkdir -p logs
LOG="logs/serial-$(date +%Y%m%d-%H%M%S).log"
ln -sf "$(basename "$LOG")" logs/latest.log
echo "logging $PORT -> $LOG"
exec uvx --with pyserial python3 - "$PORT" "$LOG" <<'EOF'
import sys, time, serial

port, path = sys.argv[1], sys.argv[2]
with open(path, "a", buffering=1) as f:
    while True:
        try:
            s = serial.Serial(port, 115200, timeout=1)
            f.write(f"--- connected {time.strftime('%F %T')} ---\n")
            while True:
                line = s.readline()
                if line:
                    f.write(time.strftime("%H:%M:%S ") + line.decode("utf-8", "replace"))
        except (serial.SerialException, OSError):
            f.write(f"--- disconnected {time.strftime('%F %T')}, retry ---\n")
            time.sleep(2)
EOF
