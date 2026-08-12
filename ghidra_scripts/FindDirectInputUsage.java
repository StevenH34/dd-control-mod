// Locates the DirectInputCreateA call site(s) and the Win32 message-pump
// calls (PeekMessageA/DispatchMessageA/TranslateMessage), bookmarking each
// under DD-DINPUT-CANDIDATE / DD-MSGLOOP-CANDIDATE. Follow-up to
// FindInputCandidates.java after confirming the game has no
// GetAsyncKeyState/GetKeyState/GetKeyboardState calls - it imports
// DirectInputCreateA instead.
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

public class FindDirectInputUsage extends GhidraScript {

    private static final String[] DINPUT_APIS = {
        "DirectInputCreateA", "DirectInputCreateW", "DirectInput8Create"
    };

    private static final String[] MSGLOOP_APIS = {
        "PeekMessageA", "DispatchMessageA", "TranslateMessage", "MapVirtualKeyA"
    };

    @Override
    public void run() throws Exception {
        println("======================================================================");
        println("DD DirectInput / message-loop scan: " + currentProgram.getName());
        println("======================================================================");

        int dinputHits = reportAndBookmark(DINPUT_APIS, "DD-DINPUT-CANDIDATE");
        int msgHits = reportAndBookmark(MSGLOOP_APIS, "DD-MSGLOOP-CANDIDATE");

        println("");
        println("======================================================================");
        println("DirectInput-related call sites bookmarked: " + dinputHits);
        println("Message-loop call sites bookmarked: " + msgHits);
        println("======================================================================");
    }

    private int reportAndBookmark(String[] apiNames, String bookmarkCategory) throws Exception {
        SymbolTable symtab = currentProgram.getSymbolTable();
        ReferenceManager refMgr = currentProgram.getReferenceManager();
        BookmarkManager bookmarkMgr = currentProgram.getBookmarkManager();
        FunctionManager funcMgr = currentProgram.getFunctionManager();

        int total = 0;
        for (String apiName : apiNames) {
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

                bookmarkMgr.setBookmark(callAddr, "Note", bookmarkCategory,
                    "call to " + apiName + " from " + funcDesc);
                total++;
            }
        }
        return total;
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
