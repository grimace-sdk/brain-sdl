#!/bin/sh
set -eu

root=$(cd "$(dirname "$0")/.." && pwd)
nm="${GRIMACE_LLVM_ROOT:-$HOME/Development/misc/grimace/llvm}/bin/llvm-nm"
library="${1:-$root/build/libSDL3.a}"

defined=$(mktemp)
"$nm" --defined-only -g "$library" 2>/dev/null | awk '$2 == "T" { print $3 }' | sort -u > "$defined"
grep -oE '^\s+SDL_[A-Za-z0-9_]+;' "$root/sources/SDL/src/dynapi/SDL_dynapi.sym" | tr -d ' \t;' | sort -u \
    | comm -12 - "$defined" > "$root/config/SDL3.exports"
rm -f "$defined"
wc -l < "$root/config/SDL3.exports"
