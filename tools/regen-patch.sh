#!/bin/sh
set -eu

root=$(cd "$(dirname "$0")/.." && pwd)
name="$1"
patch_file="$root/patches/$name/${2:-0001-windows-ce.patch}"
archive=$(python3 -I -c 'import json,sys,os; print(os.path.basename(json.load(open(sys.argv[1]))[sys.argv[2]]["url"]))' "$root/sources.json" "$name")

pristine=$(mktemp -d)
tar -xzf "$root/sources/$archive" -C "$pristine"
top=$(ls "$pristine")
mkdir -p "$(dirname "$patch_file")"
(cd "$root/sources" && diff -ruN "$pristine/$top" "$name" || true) | grep -v "^diff " | python3 -I -c '
import re, sys
text = sys.stdin.read()
text = re.sub(r"^--- \S+?/" + re.escape(sys.argv[1]) + r"/(\S+).*$", r"--- a/\1", text, flags=re.M)
text = re.sub(r"^\+\+\+ " + re.escape(sys.argv[2]) + r"/(\S+).*$", r"+++ b/\1", text, flags=re.M)
sys.stdout.write(text)
' "$top" "$name" > "$patch_file"
rm -rf "$pristine"
grep -c "^+++ " "$patch_file"
