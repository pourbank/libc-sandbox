#!/bin/bash
CLANG="/usr/bin/clang"
OPT="/usr/bin/opt" 
DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
INST_PASS_SO="$DIR/InstrumentPass.so"
CFG_PASS_SO="$DIR/SyscallPass.so"

OUTPUT=""
INPUT=""
EXTRA_FLAGS=()

while [[ $# -gt 0 ]]; do
    case "$1" in
        -o)
            shift
            OUTPUT="$1"
            ;;
        *.c)
            INPUT="$1"
            ;;
        *)
            EXTRA_FLAGS+=("$1")
            ;;
    esac
    shift
done

BC_FILE="$OUTPUT.bc"
OPT_BC_FILE="$OUTPUT.opt.bc"

mkdir -p build_bc
mkdir -p policy

$CLANG -emit-llvm -c "$INPUT" "${EXTRA_FLAGS[@]}" \
       -Wno-unused-command-line-argument -o "build_bc/$(basename $BC_FILE)" || exit 1

$OPT -load-pass-plugin="$INST_PASS_SO" \
     -passes="instrument-pass" \
     "build_bc/$(basename $BC_FILE)" \
     -o "build_bc/$(basename $OPT_BC_FILE)" || exit 1

REL_PATH=$(realpath --relative-to="$DIR" "$INPUT")
if [[ "$REL_PATH" == programs/* ]]; then
    $OPT -load-pass-plugin="$CFG_PASS_SO" \
         -passes="syscall-cfg-pass" \
         "build_bc/$(basename $OPT_BC_FILE)" \
         -disable-output || exit 1

    PROGRAM_NAME=$(basename "$INPUT" .c)
    mkdir -p policy/$PROGRAM_NAME
    mv *.dot policy/$PROGRAM_NAME/ 2>/dev/null || true
fi

$CLANG "build_bc/$(basename $OPT_BC_FILE)" "${EXTRA_FLAGS[@]}" \
       -c -Wno-unused-command-line-argument -o "$OUTPUT" || exit 1
