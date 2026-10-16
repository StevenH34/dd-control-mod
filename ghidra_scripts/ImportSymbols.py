# Applies a symbols file written by ExportSymbols.py (docs/symbols.json by
# default) to the current program: function names, labels and comments.
# Creates functions that Ghidra didn't find (e.g. ones only referenced as
# data). Existing comments of the same type at the same address are
# replaced. Safe to re-run.
#
# Refuses to run if the file was exported from a different binary (SHA-256
# mismatch), since the addresses would be wrong. Pass "force" as a second
# script arg to apply anyway.
#
# Optional script args: input path, "force". Run with PyGhidra
# (pyghidraRun.bat). Headless: drop -readOnly so the changes are saved.
#
# @category DeathtrapDungeon
# @runtime PyGhidra

import json
import os
import typing

if typing.TYPE_CHECKING:
    from ghidra.ghidra_builtins import *

from ghidra.app.util import NamespaceUtils
from ghidra.program.model.listing import CommentType
from ghidra.program.model.symbol import SourceType


def default_input_path():
    # ghidra_scripts/ImportSymbols.py -> <repo>/docs/symbols.json
    script_dir = getSourceFile().getParentFile().getAbsolutePath()
    return os.path.join(os.path.dirname(script_dir), "docs", "symbols.json")


def get_namespace(program, path):
    if not path:
        return program.getGlobalNamespace()
    return NamespaceUtils.createNamespaceHierarchy(
        path, program.getGlobalNamespace(), program, SourceType.USER_DEFINED)


def apply_symbol(program, entry):
    address = toAddr(entry["address"])
    namespace = get_namespace(program, entry["namespace"])
    name = entry["name"]

    if entry["kind"] == "function":
        fn = getFunctionAt(address)
        if fn is None:
            fn = createFunction(address, name)
            if fn is None:
                return "could not create function"
        sym = fn.getSymbol()
        if sym.getName() != name or sym.getParentNamespace() != namespace:
            sym.setNameAndNamespace(name, namespace, SourceType.USER_DEFINED)
        return None

    sym = program.getSymbolTable().createLabel(address, name, namespace, SourceType.USER_DEFINED)
    if entry.get("primary") and not sym.isPrimary():
        sym.setPrimary()
    return None


def main():
    args = [str(a) for a in getScriptArgs()]
    force = "force" in args
    args = [a for a in args if a != "force"]
    path = args[0] if args else default_input_path()

    with open(path, encoding="utf-8") as f:
        data = json.load(f)

    program = currentProgram
    sha = program.getExecutableSHA256()
    if data.get("sha256") and sha and data["sha256"].lower() != sha.lower():
        msg = "%s was exported from a different binary (%s, this is %s)." % (path, data["sha256"], sha)
        if not force:
            printerr(msg + " Pass 'force' to apply anyway.")
            return
        print("Warning: " + msg)

    applied = 0
    failed = []
    for entry in data.get("symbols", []):
        try:
            error = apply_symbol(program, entry)
        except Exception as e:  # keep going; report at the end
            error = str(e)
        if error:
            failed.append("%s %s: %s" % (entry["address"], entry["name"], error))
        else:
            applied += 1

    listing = program.getListing()
    comments = 0
    for entry in data.get("comments", []):
        listing.setComment(toAddr(entry["address"]), CommentType.valueOf(entry["type"]), entry["text"])
        comments += 1

    print("Applied %d symbols and %d comments from %s" % (applied, comments, path))
    for line in failed:
        printerr("Failed: " + line)


main()
