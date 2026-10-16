#!/bin/sh
set -eu

shell=$(pwd)/forge
tmp=$(mktemp -d)
trap 'rm -f "$tmp/output" "$tmp/stdout" "$tmp/stderr" "$tmp/error" "$tmp/both"; rmdir "$tmp"' EXIT HUP INT TERM

output=$(printf '%s\n' \
    'echo hello' \
    'printf "a b\n" | wc -c' \
    'export FORGE_TEST=value' \
    'echo "$FORGE_TEST"' \
    "echo '\$FORGE_TEST'" \
    'echo escaped\ space' \
    'false' \
    'echo $?' \
    'echo first > "'"$tmp"'/output"' \
    'echo second >> "'"$tmp"'/output"' \
    'cat < "'"$tmp"'/output"' \
    "sh -c 'echo captured >&2' 2> '$tmp/error'" \
    "sh -c 'echo stdout; echo stderr >&2' &> '$tmp/both'" \
    'exit 0' | "$shell")

printf '%s\n' "$output" | grep -q '^hello$'
printf '%s\n' "$output" | grep -Eq '^[[:space:]]*4$'
printf '%s\n' "$output" | grep -q '^value$'
printf '%s\n' "$output" | grep -q '^\$FORGE_TEST$'
printf '%s\n' "$output" | grep -q '^escaped space$'
printf '%s\n' "$output" | grep -q '^1$'
printf '%s\n' "$output" | grep -q '^first$'
printf '%s\n' "$output" | grep -q '^second$'
grep -q '^captured$' "$tmp/error"
grep -q '^stdout$' "$tmp/both"
grep -q '^stderr$' "$tmp/both"

if printf 'echo "unterminated\n' | "$shell" >"$tmp/stdout" 2>"$tmp/stderr"; then
    echo "unmatched quote unexpectedly succeeded" >&2
    exit 1
fi
grep -q 'unmatched quote' "$tmp/stderr"

printf 'forge tests passed\n'
