#!/usr/bin/env bash
# Style gate for src/ and tests/ (see AGENTS.md, 代码风格): member prefixes, address placeholder names, Hungarian
# prefixes and C-style memory management. Prints offending lines and exits non-zero when any are found.
set -u
cd "$(dirname "$0")/.."
status=0
check() {
    local label="$1" pattern="$2" allowed="${3:-$^}"
    local hits
    hits=$(rg -n --no-heading -e "$pattern" src tests --glob '*.h' --glob '*.cpp' | rg -v '^\S+:\d+:\s*//' | rg -v -e "$allowed" || true)
    if [ -n "$hits" ]; then
        echo "== $label"
        echo "$hits"
        status=1
    fi
}
check "member prefix m_/g_/s_" '\b[mgs]_[a-zA-Z]\w*'
check "address placeholder name" '\b(dword|word|byte|unk|sub|off|loc|asc|stru)_[0-9A-Fa-f]{5,}\b'
check "Hungarian prefix" '\b(dw|lp|sz|psz|lpsz|n|p|b|h|ul|us|ui|w|fn)[A-Z][a-z]\w*'
check "std fixed-width integer" '\bstd::u?int(8|16|32|64)_t\b' 'using [ui][0-9]+ = std::'
check "malloc/free/raw delete" '\b(std::)?(malloc|calloc|realloc|free)\(|\bdelete\s*\[?\]?\s*\w'
long=$(find src tests -name "*.h" -o -name "*.cpp" | xargs awk "length > 120 { print FILENAME \":\" FNR }")
if [ -n "$long" ]; then
    echo "== line longer than 120"
    echo "$long"
    status=1
fi
exit $status
