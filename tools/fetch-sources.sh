#!/bin/sh
set -eu

root=$(cd "$(dirname "$0")/.." && pwd)
sources="$root/sources"
mkdir -p "$sources"

python3 -I -c '
import json, sys
for name, entry in json.load(open(sys.argv[1])).items():
    print(name, entry["url"], entry["sha256"])
' "$root/sources.json" | while read -r name url sha256; do
    if [ $# -gt 0 ] && ! printf "%s\n" "$@" | grep -qx "$name"; then
        continue
    fi
    archive="$sources/$(basename "$url")"
    if [ ! -f "$archive" ]; then
        curl -fsSL -o "$archive.part" "$url"
        mv "$archive.part" "$archive"
    fi
    echo "$sha256  $archive" | shasum -a 256 -c -

    unpack="$sources/.unpack-$name"
    rm -rf "$unpack" "$sources/$name"
    mkdir -p "$unpack"
    tar -xzf "$archive" -C "$unpack"
    mv "$unpack"/* "$sources/$name"
    rmdir "$unpack"

    for patch in "$root"/patches/"$name"/*.patch; do
        [ -e "$patch" ] || continue
        patch -d "$sources/$name" -p1 --forward --quiet < "$patch"
    done
done
