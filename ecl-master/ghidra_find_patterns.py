# Ghidra headless script - find functions by string xrefs and extract patterns
# Run: analyzeHeadless <proj_dir> <proj_name> -import <file> -processor x86:LE:64:default -postScript ghidra_find_patterns.py

from ghidra.program.model.symbol import RefType
from ghidra.program.model.listing import CodeUnit
import ghidra.program.model.mem as mem

def get_bytes_at(addr, count):
    """Read raw bytes from address"""
    try:
        result = bytearray(count)
        listing = currentProgram.getListing()
        code_unit = listing.getCodeUnitAt(addr)
        if code_unit:
            code_unit.getBytes(result)
            return bytes(result)
        return None
    except:
        return None

def scan_bytes(data, pattern_str):
    """Scan byte data for pattern with wildcards"""
    parts = pattern_str.split(" ")
    results = []
    for i in range(len(data) - len(parts) + 1):
        match = True
        for j, p in enumerate(parts):
            if p == "?":
                continue
            if data[i + j] != int(p, 16):
                match = False
                break
        if match:
            results.append(i)
    return results

def find_function_containing(addr):
    """Find the function containing the given address"""
    fm = currentProgram.getFunctionManager()
    func = fm.getFunctionContaining(addr)
    return func

def dump_function_bytes(func, max_bytes=128):
    """Dump first N bytes of a function"""
    entry = func.getEntryPoint()
    data = get_bytes_at(entry, max_bytes)
    return entry, data

def format_pattern(data, offset=0, length=32):
    """Format bytes as pattern string"""
    parts = []
    for i in range(length):
        if offset + i < len(data):
            parts.append("%02X" % data[offset + i])
        else:
            parts.append("??")
    return " ".join(parts)

def find_xrefs_to_string(search_str):
    """Find all code references to a string"""
    refs = []
    st = currentProgram.getSymbolTable()
    symbols = list(st.findSymbols(search_str, False))
    for sym in symbols:
        ref_mgr = currentProgram.getReferenceManager()
        refs_to = ref_mgr.getReferencesTo(sym.getAddress())
        for ref in refs_to:
            refs.append(ref)
    return refs

def try_find_function_by_string(search_str, label):
    """Try to find a function by string reference"""
    print("\n--- Searching for '%s' (%s) ---" % (search_str, label))
    
    refs = find_xrefs_to_string(search_str)
    if not refs:
        print("  No references found to string '%s'" % search_str)
        return None
    
    for ref in refs:
        from_addr = ref.getFromAddress()
        func = find_function_containing(from_addr)
        if func:
            entry, data = dump_function_bytes(func, 128)
            print("  String ref at %s, function: %s at %s" % (from_addr, func.getName(), entry))
            if data:
                pattern = format_pattern(data, 0, 16)
                print("  First 16 bytes: %s" % pattern)
                print("  FULL 32 bytes:  %s" % format_pattern(data, 0, 32))
            return func
    
    return None

print("=" * 70)
print("  GHIDRA FUNCTION FINDER")
print("  Program: %s" % currentProgram.getName())
print("=" * 70)

listing = currentProgram.getListing()
fm = currentProgram.getFunctionManager()

# === client_client.so searches ===
if "client" in currentProgram.getName().lower():
    # 1. renderBeams - search for "RenderBeams" and "DrawBeams"
    try_find_function_by_string("RenderBeams", "renderBeams")
    try_find_function_by_string("DrawBeams", "renderBeams")
    
    # 2. glowManager - search for CGlowObjectManager
    try_find_function_by_string("CGlowObjectManager", "glowManager")
    
    # 3. initKeyValues - search for KeyValues
    try_find_function_by_string("KeyValues::InitKeyValues", "initKeyValues")
    try_find_function_by_string("InitKeyValues", "initKeyValues")
    
    # 4. loadFromBuffer - search for LoadFromBuffer
    try_find_function_by_string("LoadFromBuffer", "loadFromBuffer")
    
    # 5. predictionSeed - search for prediction seed strings
    try_find_function_by_string("m_nPredictionSeed", "predictionSeed")
    try_find_function_by_string("PredictionSeed", "predictionSeed")
    
    # 6. moveData - search for MoveData
    try_find_function_by_string("CMoveData", "moveData")
    try_find_function_by_string("m_pMoveData", "moveData")
    
    # 7. restoreEntity - search for RestoreEntity
    try_find_function_by_string("RestoreEntity", "restoreEntity")
    try_find_function_by_string("restoreToPredictedFrame", "restoreEntity")
    
    # 8. lineGoesThroughSmoke - search for smoke-related strings
    try_find_function_by_string("LineGoesThroughSmoke", "lineGoesThruSmoke")
    try_find_function_by_string("line_goes_through_smoke", "lineGoesThruSmoke")

# === engine_client.so searches ===
elif "engine" in currentProgram.getName().lower():
    # sendClantag
    try_find_function_by_string("ClanTag", "sendClantag")
    try_find_function_by_string("SetClanTag", "sendClantag")
    try_find_function_by_string("send_clan_tag", "sendClantag")
    
    # hostSecure  
    try_find_function_by_string("Host_IsSecureServerAllowed", "hostSecure")
    try_find_function_by_string("host_issecure", "hostSecure")
    
    # setNamedSkybox
    try_find_function_by_string("SetNamedSkybox", "setNamedSkybox")
    try_find_function_by_string("set_named_skybox", "setNamedSkybox")

print("\n" + "=" * 70)
print("  DONE")
print("=" * 70)
