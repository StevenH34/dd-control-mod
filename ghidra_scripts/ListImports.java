// Dumps every imported (external) symbol name in the current program.
// One-off diagnostic to figure out which Win32 subsystem(s) the game uses
// for input (DirectInput vs raw window messages vs something else), since
// FindInputCandidates.java found no GetAsyncKeyState/GetKeyState/
// GetKeyboardState calls in DUNGEON.DLL.
//
// @category DeathtrapDungeon

import ghidra.app.script.GhidraScript;
import ghidra.program.model.symbol.Symbol;
import ghidra.program.model.symbol.SymbolIterator;
import ghidra.program.model.symbol.SymbolTable;

import java.util.ArrayList;
import java.util.Collections;
import java.util.List;

public class ListImports extends GhidraScript {

    @Override
    public void run() throws Exception {
        SymbolTable symtab = currentProgram.getSymbolTable();

        List<String> names = new ArrayList<>();
        SymbolIterator it = symtab.getExternalSymbols();
        while (it.hasNext()) {
            Symbol s = it.next();
            names.add(s.getName());
        }
        Collections.sort(names);

        println("======================================================================");
        println("Imports for " + currentProgram.getName() + " (" + names.size() + " total)");
        println("======================================================================");
        for (String n : names) {
            println(n);
        }
    }
}
