"""Puts a blank line before and after every braced block (AGENTS.md, 代码风格), after clang-format has placed the
braces on their own lines. Blank lines are not added at the start or end of an enclosing block, between a closing
brace and its continuation (else, catch, while of do-while, closing parenthesis), or inside argument lists.
Usage: python3 scripts/blank_lines.py <file>..."""

import sys


def indent(line):
    return len(line) - len(line.lstrip(" "))


def header_start(lines, brace):
    level = indent(lines[brace])
    start = brace - 1
    while start >= 0:
        text = lines[start].strip()
        if text and (indent(lines[start]) > level or text.startswith(")")):
            start -= 1
            continue
        break
    if start < 0 or not lines[start].strip() or indent(lines[start]) != level:
        return None
    while start > 0:
        above = lines[start - 1]
        text = above.strip()
        if indent(above) == level and (text.startswith("//") or text.startswith("template")):
            start -= 1
            continue
        break
    return start


def opens_block(text):
    return text.endswith("{") or text.endswith(":") or text.endswith("(") or text.endswith(",")


def process(lines):
    before = set()
    after = set()
    for number, line in enumerate(lines):
        text = line.strip()
        if text == "{":
            start = header_start(lines, number)
            if start is None or start == 0:
                continue
            header = lines[start].strip()
            previous = lines[start - 1].strip()
            if not previous or opens_block(previous) or previous.startswith("#"):
                continue
            if header.startswith(("else", "catch")) or previous.startswith("}") and header.startswith("while"):
                continue
            before.add(start)
        elif text.startswith("}"):
            if number + 1 >= len(lines):
                continue
            following = lines[number + 1].strip()
            if not following or following.startswith(("}", ")", "else", "catch", "#", ",")):
                continue
            if following.startswith("while") and following.endswith(";"):
                continue
            after.add(number)
    result = []
    for number, line in enumerate(lines):
        if number in before and result and result[-1].strip():
            result.append("\n")
        if line.strip() or not result or result[-1].strip():
            result.append(line)
        if number in after:
            result.append("\n")
    return result


def main():
    for path in sys.argv[1:]:
        with open(path, encoding="utf-8") as source:
            lines = source.readlines()
        updated = process(lines)
        if updated != lines:
            with open(path, "w", encoding="utf-8", newline="\n") as target:
                target.writelines(updated)


if __name__ == "__main__":
    main()
