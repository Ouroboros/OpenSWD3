#!/usr/bin/env bash
# Registers the functions defined in the given source files in reverse/functions.tsv: every definition preceded by a
# "// swd32.exe 0x<address>" comment sets module, source (path:symbol) and, when still pending, the status
# implemented_pending_review. Consecutive address comments (an EH funclet after its parent) all map to the definition
# that follows. Usage: scripts/register.sh <module> <file>...
set -eu
cd "$(dirname "$0")/.."
module="$1"
shift
map="tmp/register_map.tsv"
: > "$map"
for file in "$@"; do
    awk -v file="$file" -v mod="$module" '
        /^(\/\/|    \/\/) swd32\.exe 0x/ {
            match($0, /0x[0-9A-F]+/); count = 1; address[1] = substr($0, RSTART, RLENGTH)
            while ((getline line) > 0) {
                if (line ~ /^\s*\/\/ swd32\.exe 0x/) { match(line, /0x[0-9A-F]+/); address[++count] = substr(line, RSTART, RLENGTH); continue }
                if (line ~ /^\s*\/\// || line ~ /^\s*template/) continue
                if (match(line, /[A-Za-z_0-9:~]+\(/)) {
                    for (i = 1; i <= count; ++i) print address[i] "\t" mod "\t" file ":" substr(line, RSTART, RLENGTH - 1)
                }
                break
            }
        }' "$file" >> "$map"
done
awk -F'\t' -v OFS='\t' 'FILENAME == ARGV[1] { mod[$1] = $2; src[$1] = $3; next }
    ($3 in mod) { $5 = mod[$3]; $6 = src[$3]; if ($7 == "pending_implementation") $7 = "implemented_pending_review" }
    { print }' "$map" reverse/functions.tsv > tmp/register_functions.tsv
mv tmp/register_functions.tsv reverse/functions.tsv
cat "$map"
rm -f "$map"
awk -F'\t' 'NR > 1 { count[$7]++ } END { for (k in count) print k, count[k] }' reverse/functions.tsv
