#!/usr/bin/env bash
# Regenerates docs/images/ from the real UI code with made-up drives.
# Needs: gcc, python3 with Pillow and fonttools, the JetBrains Mono and DejaVu Sans Mono fonts.
set -euo pipefail
cd "$(dirname "$0")/../.."

OUT=build/screenshots
FLAGS=(-O1 -D_DEFAULT_SOURCE -D_POSIX_C_SOURCE=200112L -pthread)
mkdir -p "$OUT" docs/images

mapfile -t SRCS < <(find src -name '*.c' ! -path src/main.c)
gcc "${FLAGS[@]}" scripts/screenshots/demo.c "${SRCS[@]}" -Wl,--wrap=statvfs -lcrypto -ludev -lblkid -latomic -o "$OUT/demo"

for screen in list nvme ata info; do "$OUT/demo" "$screen" > "$OUT/$screen.ans" 2>&1 < /dev/null; done
"$OUT/demo" clone > "$OUT/clone.ans" 2>&1 <<< y

python3 scripts/screenshots/render.py "$OUT" docs/images
echo "Wrote docs/images/"
