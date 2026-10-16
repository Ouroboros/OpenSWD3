#!/usr/bin/env bash
# Registers the functions defined in the given source files in reverse/functions.tsv: every definition preceded by a
# "// swd32.exe 0x<address>" comment sets module, source (path:symbol) and, when still pending, the status
# implemented_pending_review. The text after "0x<address>:" with the comment lines that follow it becomes the notes
# of the function, followed by the notes the row had before as "[prior: ...]" ("none" for none). Consecutive address
# comments (an EH funclet after its parent) all map to the definition that follows.
# Usage: scripts/register.sh <module> <file>...
set -eu
cd "$(dirname "$0")/.."
module="$1"
shift
map="tmp/register_map.tsv"
: > "$map"
for file in "$@"; do
    awk -v file="$file" -v mod="$module" '
        function describe(text) {
            sub(/^\s*\/\/ swd32\.exe 0x[0-9A-F]+:? ?/, "", text)
            return text
        }
        /^(\/\/|    \/\/) swd32\.exe 0x/ {
            match($0, /0x[0-9A-F]+/); count = 1; address[1] = substr($0, RSTART, RLENGTH)
            note = describe($0)
            while ((getline line) > 0) {
                if (line ~ /^\s*\/\/ swd32\.exe 0x/) {
                    match(line, /0x[0-9A-F]+/); address[++count] = substr(line, RSTART, RLENGTH)
                    more = describe(line)
                    if (more != "") note = note (note == "" ? "" : " ") more
                    continue
                }
                if (line ~ /^\s*\/\//) {
                    sub(/^\s*\/\/ ?/, "", line)
                    note = note (note == "" ? "" : " ") line
                    continue
                }
                if (line ~ /^\s*template/) continue
                if (match(line, /[A-Za-z_0-9:~]+\(/)) {
                    gsub(/\t/, " ", note)
                    for (i = 1; i <= count; ++i) print address[i] "\t" mod "\t" file ":" substr(line, RSTART, RLENGTH - 1) "\t" note
                }
                break
            }
        }' "$file" >> "$map"
done
awk -F'\t' -v OFS='\t' 'FILENAME == ARGV[1] { mod[$1] = $2; src[$1] = $3; note[$1] = $4; next }
    ($3 in mod) {
        $5 = mod[$3]; $6 = src[$3]
        if ($7 == "pending_implementation") $7 = "implemented_pending_review"
        prior = $10
        if (match(prior, /\[prior: .*\]$/)) prior = substr(prior, RSTART + 8, RLENGTH - 9)
        if (prior == "") prior = "none"
        $10 = note[$3] (note[$3] == "" ? "" : " ") "[prior: " prior "]"
    }
    { print }' "$map" reverse/functions.tsv > tmp/register_functions.tsv
mv tmp/register_functions.tsv reverse/functions.tsv
cut -f1-3 "$map"
rm -f "$map"
awk -F'\t' 'NR > 1 { count[$7]++ } END { for (k in count) print k, count[k] }' reverse/functions.tsv
