# Ghidra headless script - pattern scanner
# Run: analyzeHeadless <project_dir> <project_name> -import <file> -postScript ghidra_scan.py
# Or use with: analyzeHeadless ... -noanalysis -readOnly -postScript ghidra_scan.py

from ghidra.program.model.listing import CodeUnit
from ghidra.program.model.mem import MemoryAccessException
import re

PATTERNS = {
    "client_client.so": {
        "renderBeams":         "4C 89 F6 4C 8B 25 ? ? ? ? 48 8D 05",
        "glowManager":         "E8 ? ? ? ? 48 8B 3D ? ? ? ? BE 01 00 00 00 C7",
        "initKeyValues":       "81 27 00 00 00 FF 55 31 C0 48 89 E5 5D",
        "loadFromBuffer":      "55 48 89 E5 41 57 41 56 41 55 41 54 49 89 D4 53 48 81 EC ? ? ? ? 48 85",
        "predictionSeed":      "48 8B 05 ? ? ? ? 8B 38 E8 ? ? ? ? 89 C7",
        "moveData":            "48 8B 0D ? ? ? ? 4C 89 EA",
        "restoreEntity":       "55 48 89 E5 41 57 41 89 D7 41 56 41 55 41 89 F5 41 54 53 48 83 EC 18",
        "lineGoesThruSmoke":   "55 48 89 E5 41 56 41 55 41 54 53 48 83 EC 30 66 0F D6 45 D0",
    },
    "engine_client.so": {
        "hostSecure":          "55 48 89 E5 E8 ? ? ? ? 48 8D 35 ? ? ? ? 48 8B 10 48 89 C7 FF 52 58 85 C0 74 13",
        "sendClantag":         "55 48 89 E5 41 55 49 89 FD 41 54 BF",
        "setNamedSkybox":      "55 4C 8D 05 ? ? ? ? 48 89 E5 41",
    }
}

def pattern_to_regex(pattern_str):
    bytes_list = pattern_str.split(" ")
    regex = ""
    for b in bytes_list:
        if b == "?":
            regex += "."
        else:
            regex += chr(92) + "x" + b
    return regex

def scan_memory(program, pattern_str):
    regex = pattern_to_regex(pattern_str)
    regex_bytes = regex.replace(chr(92) + "x", "").decode("hex") if False else None
    
    results = []
    memory = program.getMemory()
    
    for block in memory.getBlocks():
        if not block.isInitialized() or not block.isExecute():
            continue
        try:
            start = block.getStart().getOffset()
            size = block.getSize()
            data = bytearray(block.getBytes(block.getStart(), size))
            
            pat_bytes = pattern_str.split(" ")
            for i in range(len(data) - len(pat_bytes) + 1):
                match = True
                for j, p in enumerate(pat_bytes):
                    if p == "?":
                        continue
                    if data[i + j] != int(p, 16):
                        match = False
                        break
                if match:
                    addr = program.getAddressFactory().getAddress("0x" + format(start + i, "x"))
                    results.append(addr)
        except:
            continue
    
    return results

def get_func_at_addr(program, addr):
    fm = program.getFunctionManager()
    func = fm.getFunctionAt(addr)
    if func:
        return func.getName()
    return None

def get_function_bytes(func, count=64):
    body = func.getBody()
    inst_iter = listing.getInstructions(body.getMinAddress(), True)
    result = []
    addr = func.getEntryPoint()
    while inst_iter.hasNext() and len(result) < count:
        inst = inst_iter.next()
        if not body.contains(inst.getAddress()):
            break
        result.append((inst.getAddress(), inst.getMnemonicString(), str(inst.getDefaultOperandRepresentationList())))
    return result

print("=" * 60)
print("PATTERN SCAN RESULTS")
print("=" * 60)

listing = currentProgram.getListing()
file_name = str(currentProgram.getExecutablePath()).split("/")[-1].split("\\")[-1]

if file_name in PATTERNS:
    for name, pattern in PATTERNS[file_name].items():
        results = scan_memory(currentProgram, pattern)
        if results:
            print("[FOUND]   %-25s %s -> %s" % (name, file_name, ", ".join([str(r) for r in results])))
        else:
            print("[MISSING] %-25s %s" % (name, file_name))

print("=" * 60)
print("FUNCTION SEARCH FOR MISSING PATTERNS")
print("=" * 60)

# Search for functions by known string references
string_searches = {
    "KeyValues::InitKeyValues": ["KeyValues", "init"],
    "KeyValues::LoadFromBuffer": ["LoadFromBuffer", "LoadFromFile"],
    "LineGoesThroughSmoke": ["LineGoesThroughSmoke", "line_goes_through_smoke"],
    "SendClantag": ["SendClanTag", "clan_tag", "SetClanTag"],
    "CGlowObjectManager": ["CGlowObjectManager", "glow_manager"],
    "CRenderBeams": ["CRenderBeams", "RenderBeams", "DrawBeams"],
    "CMoveData": ["CMoveData", "MoveData"],
    "CPrediction": ["CPrediction::GetPred", "m_nPredictionSeed", "pred_seed"],
}

fm = currentProgram.getFunctionManager()
memory = currentProgram.getMemory()

# Try to find functions by scanning all symbols
print("\nSearching for named symbols...")
symbol_table = currentProgram.getSymbolTable()
found_symbols = {}
for symbol in symbol_table.getAllSymbols(True):
    sname = str(symbol.getName()).lower()
    for key in ["keyvalues", "initkeyvalues", "loadfrombuffer", "linegoesthroughsmoke",
                 "line_goes_through_smoke", "sendclantag", "setclantag", "clan",
                 "glowobject", "glowmanager", "renderbeams", "c_render_beams",
                 "movedata", "m_nmovedata", "predictionseed", "predseed",
                 "restoreentity", "restoretopredicted"]:
        if key in sname:
            found_symbols[str(symbol.getName())] = str(symbol.getAddress())
            print("  SYMBOL: %s -> %s" % (symbol.getName(), symbol.getAddress()))

if not found_symbols:
    print("  No matching symbols found (binary is likely stripped)")

print("\nSearching for string references...")
search_strings = ["KeyValues", "LoadFromBuffer", "LineGoesThroughSmoke",
                  "clan_tag", "CGlowObjectManager", "CMoveData", "m_nPredictionSeed",
                  "restoreEntity", "DrawBeams", "render_beams"]

for s in search_strings:
    for ref in getReferencesTo(currentProgram.getSymbolTable().findSymbols(s, False)):
        if ref:
            print("  STRING '%s' referenced at %s" % (s, ref.getFromAddress()))

print("=" * 60)
print("SCAN COMPLETE")
print("=" * 60)
