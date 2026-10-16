# Exports the names and comments you added in Ghidra to a text file in the
# repo (docs/symbols.json by default), so the annotations can be versioned
# without publishing the Ghidra project. ImportSymbols.py applies them back.
#
# Exported: user-defined function names and labels (with their namespace),
# and comments (plate, pre, EOL, post, repeatable). Names that come from the
# binary itself (exports, imports) and Ghidra's default/analysis names are
# skipped, and so are the comments Ghidra adds by itself (see
# AUTO_COMMENT_PATTERNS); it recreates those on import anyway. No code or
# bytes from the program are written.
#
# Optional script args: output path, "all" (keep automatic comments too).
# Run with PyGhidra (pyghidraRun.bat).
#
# @category DeathtrapDungeon
# @runtime PyGhidra

import json
import os
import re
import typing

if typing.TYPE_CHECKING:
    from ghidra.ghidra_builtins import *

from ghidra.program.model.listing import CommentType
from ghidra.program.model.symbol import SourceType, SymbolType

COMMENT_TYPES = [CommentType.PLATE, CommentType.PRE, CommentType.EOL,
                 CommentType.POST, CommentType.REPEATABLE]

# Comments written by Ghidra's PE loader and Function ID analyzer, by type.
AUTO_COMMENT_PATTERNS = {
    "PLATE": [
        re.compile(r"^Library Function - "),             # Function ID match
        re.compile(r"^IMAGE_[A-Z0-9_]+"),                # PE structures
        re.compile(r"^Export (Function Pointers|Name Pointers|Ordinal Values|Library Name)$"),
        re.compile(r"^Rsrc_\S+ Size of resource: 0x[0-9a-fA-F]+ bytes$"),  # PE resources
    ],
    "PRE": [
        re.compile(r"^0x[0-9a-f]+\s+\d+\s+\S+$"),          # export: RVA, ordinal, name
        re.compile(r"^\d+\s+\S+\s+<<[^>]*>>$"),           # import: hint, name, <<not bound>>
    ],
    "EOL": [
        re.compile(r"^IMAGE_[A-Z0-9_]+$"),               # PE structures
    ],
}


def is_auto_comment(program, address, comment_type, text):
    for pattern in AUTO_COMMENT_PATTERNS.get(comment_type.name(), []):
        if pattern.match(text):
            return True
    # Section names the loader puts on the PE header's section table.
    block = program.getMemory().getBlock(address)
    if comment_type == CommentType.EOL and block is not None and block.getName() == "Headers":
        return re.match(r"^\.\w+$", text) is not None
    return False


def default_output_path():
    # ghidra_scripts/ExportSymbols.py -> <repo>/docs/symbols.json
    script_dir = getSourceFile().getParentFile().getAbsolutePath()
    return os.path.join(os.path.dirname(script_dir), "docs", "symbols.json")


def addr_str(address):
    return "%08x" % address.getOffset()


def collect_symbols(program):
    symbols = []
    table = program.getSymbolTable()
    for sym in table.getAllSymbols(True):
        if sym.getSource() != SourceType.USER_DEFINED:
            continue
        sym_type = sym.getSymbolType()
        if sym_type == SymbolType.FUNCTION:
            kind = "function"
        elif sym_type == SymbolType.LABEL:
            kind = "label"
        else:
            continue
        if sym.isExternal():
            continue
        namespace = sym.getParentNamespace()
        symbols.append({
            "address": addr_str(sym.getAddress()),
            "kind": kind,
            "name": sym.getName(),
            "namespace": "" if namespace.isGlobal() else namespace.getName(True),
            "primary": sym.isPrimary(),
        })
    symbols.sort(key=lambda s: (s["address"], s["kind"] != "function", s["name"]))
    return symbols


def collect_comments(program, keep_auto):
    listing = program.getListing()
    comments = []
    skipped = 0
    addresses = listing.getCommentAddressIterator(program.getMemory(), True)
    for address in addresses:
        for comment_type in COMMENT_TYPES:
            text = listing.getComment(comment_type, address)
            if not text:
                continue
            if not keep_auto and is_auto_comment(program, address, comment_type, text):
                skipped += 1
                continue
            comments.append({
                "address": addr_str(address),
                "type": comment_type.name(),
                "text": text,
            })
    return comments, skipped


def write_json(path, data):
    # One entry per line keeps diffs readable.
    lines = ["{"]
    lines.append('  "program": %s,' % json.dumps(data["program"]))
    lines.append('  "sha256": %s,' % json.dumps(data["sha256"]))
    lines.append('  "image_base": %s,' % json.dumps(data["image_base"]))
    for key in ("symbols", "comments"):
        entries = data[key]
        lines.append('  "%s": [' % key)
        for i, entry in enumerate(entries):
            comma = "," if i < len(entries) - 1 else ""
            lines.append("    " + json.dumps(entry, ensure_ascii=False) + comma)
        lines.append("  ]" + ("," if key == "symbols" else ""))
    lines.append("}")
    with open(path, "w", encoding="utf-8", newline="\n") as f:
        f.write("\n".join(lines) + "\n")


def main():
    args = [str(a) for a in getScriptArgs()]
    keep_auto = "all" in args
    args = [a for a in args if a != "all"]
    path = args[0] if args else default_output_path()

    program = currentProgram
    comments, skipped = collect_comments(program, keep_auto)
    data = {
        "program": program.getName(),
        "sha256": program.getExecutableSHA256(),
        "image_base": addr_str(program.getImageBase()),
        "symbols": collect_symbols(program),
        "comments": comments,
    }

    os.makedirs(os.path.dirname(os.path.abspath(path)), exist_ok=True)
    write_json(path, data)
    print("Exported %d symbols and %d comments to %s (skipped %d automatic comments)"
          % (len(data["symbols"]), len(data["comments"]), path, skipped))


main()
