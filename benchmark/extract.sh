#!/bin/bash

set -e

BUILD_PROGRAMS="mbedtls/build/programs"
OUTPUT_PROGRAMS="programs"
OUTPUT_POLICY="policy"

ROOTFS_DIR="../kernel/rootfs"

mkdir -p "$OUTPUT_PROGRAMS"
mkdir -p "$OUTPUT_POLICY"

echo "Copying executables..."
find "$BUILD_PROGRAMS" -mindepth 1 -maxdepth 2 -type f -executable | while read exe; do
    cp "$exe" "$OUTPUT_PROGRAMS/"
done

echo "Collecting .dot files..."
find "$BUILD_PROGRAMS" -mindepth 2 -type f -name '*.dot' | while read dotfile; do
    cp "$dotfile" "$OUTPUT_POLICY/"
done

echo "Extraction complete!"
echo "Executables: $OUTPUT_PROGRAMS"
echo ".dot files: $OUTPUT_POLICY"

echo "Converting .dot files to .bin policies..."
python3 CreatePolicy.py

echo "Moving programs and policies into rootfs..."
cp -r "$OUTPUT_PROGRAMS" "$ROOTFS_DIR/"
cp -r "$OUTPUT_POLICY" "$ROOTFS_DIR/"


