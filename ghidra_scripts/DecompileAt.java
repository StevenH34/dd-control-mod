// Decompiles the function(s) at the given address(es) and prints the C
// output. Addresses are passed as script args, comma-or-space separated,
// e.g.: -postScript DecompileAt.java "100a9de8,10074860"
//
// @category DeathtrapDungeon

import ghidra.app.decompiler.DecompInterface;
import ghidra.app.decompiler.DecompileResults;
import ghidra.app.script.GhidraScript;
import ghidra.program.model.address.Address;
import ghidra.program.model.listing.Function;
import ghidra.util.task.ConsoleTaskMonitor;

public class DecompileAt extends GhidraScript {

    @Override
    public void run() throws Exception {
        String[] args = getScriptArgs();
        if (args.length == 0) {
            println("Usage: DecompileAt.java <addr1,addr2,...>");
            return;
        }

        String joined = String.join(",", args);
        String[] addrs = joined.split("[,\\s]+");

        DecompInterface decomp = new DecompInterface();
        decomp.openProgram(currentProgram);

        for (String addrStr : addrs) {
            if (addrStr.isEmpty()) continue;
            Address addr = currentProgram.getAddressFactory().getAddress(addrStr);
            Function func = currentProgram.getFunctionManager().getFunctionAt(addr);
            if (func == null) {
                func = currentProgram.getFunctionManager().getFunctionContaining(addr);
            }
            if (func == null) {
                println("No function at/containing " + addrStr);
                continue;
            }

            println("======================================================================");
            println("Function: " + func.getName() + " @ " + func.getEntryPoint());
            println("======================================================================");

            DecompileResults results = decomp.decompileFunction(func, 60, new ConsoleTaskMonitor());
            if (results != null && results.decompileCompleted()) {
                println(results.getDecompiledFunction().getC());
            } else {
                println("Decompilation failed: " + (results != null ? results.getErrorMessage() : "unknown error"));
            }
        }

        decomp.dispose();
    }
}
