// Lists all callers of the Win32 keyboard-polling APIs (GetAsyncKeyState,
// GetKeyState, GetKeyboardState) imported by the current program, as a
// starting candidate list for Phase 1 input-handling discovery.
//
// Run with Dungeon.DLL as the active program. Bookmarks each call site so
// it's easy to jump back to in the Ghidra GUI (Window > Bookmarks), and
// prints a table to the console.
//
// This only finds *statically imported* calls. If the game reads input via
// DirectInput (vtable calls through IDirectInputDevice::GetDeviceState),
// those won't show up here - if this script finds nothing, that's the next
// thing to check by hand (look for imports of dinput.dll / DirectInput8Create).
//
// @category DeathtrapDungeon

import ghidra.app.script.GhidraScript;
import ghidra.program.model.address.Address;
import ghidra.program.model.listing.BookmarkManager;
import ghidra.program.model.listing.Function;
import ghidra.program.model.listing.FunctionManager;
import ghidra.program.model.symbol.Reference;
import ghidra.program.model.symbol.ReferenceIterator;
import ghidra.program.model.symbol.ReferenceManager;
import ghidra.program.model.symbol.Symbol;
import ghidra.program.model.symbol.SymbolIterator;
import ghidra.program.model.symbol.SymbolTable;

import java.util.ArrayList;
import java.util.List;

public class FindInputCandidates extends GhidraScript {

    private static final String[] CANDIDATE_APIS = {
        "GetAsyncKeyState", "GetKeyState", "GetKeyboardState"
    };

    @Override
    public void run() throws Exception {
        SymbolTable symtab = currentProgram.getSymbolTable();
        ReferenceManager refMgr = currentProgram.getReferenceManager();
        BookmarkManager bookmarkMgr = currentProgram.getBookmarkManager();
        FunctionManager funcMgr = currentProgram.getFunctionManager();

        println("======================================================================");
        println("DD input-candidate scan: " + currentProgram.getName());
        println("======================================================================");

        int totalHits = 0;

        for (String apiName : CANDIDATE_APIS) {
            Symbol sym = findImportSymbol(symtab, apiName);
            if (sym == null) {
                println("[" + apiName + "] not imported by this program");
                continue;
            }

            Address entry = sym.getAddress();
            ReferenceIterator refs = refMgr.getReferencesTo(entry);

            List<Reference> hits = new ArrayList<>();
            while (refs.hasNext()) {
                hits.add(refs.next());
            }

            println("");
            println("[" + apiName + "] imported at " + entry + " - " + hits.size() + " call site(s)");

            for (Reference ref : hits) {
                Address callAddr = ref.getFromAddress();
                Function containingFunc = funcMgr.getFunctionContaining(callAddr);
                String funcDesc = containingFunc != null ? containingFunc.getName() : "<no function>";
                Address funcAddr = containingFunc != null ? containingFunc.getEntryPoint() : callAddr;

                println("    call at " + callAddr + "  in function " + funcDesc + " @ " + funcAddr);

                bookmarkMgr.setBookmark(callAddr, "Note", "DD-INPUT-CANDIDATE",
                    "call to " + apiName + " from " + funcDesc);
                totalHits++;
            }
        }

        println("");
        println("======================================================================");
        println("Total candidate call sites bookmarked: " + totalHits);
        println("Open Window > Bookmarks and filter category 'DD-INPUT-CANDIDATE' to review.");
        if (totalHits == 0) {
            println("No GetAsyncKeyState/GetKeyState/GetKeyboardState calls found -");
            println("the game likely reads input via DirectInput instead. Check imports");
            println("for dinput.dll / DirectInput8Create and follow the returned");
            println("IDirectInputDevice interface's GetDeviceState vtable slot instead.");
        }
        println("======================================================================");
    }

    private Symbol findImportSymbol(SymbolTable symtab, String name) {
        SymbolIterator it = symtab.getSymbols(name);
        while (it.hasNext()) {
            Symbol s = it.next();
            if (s.isExternal() || s.isGlobal()) {
                return s;
            }
        }
        return null;
    }
}
