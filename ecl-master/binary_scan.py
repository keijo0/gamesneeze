#!/usr/bin/env python3
"""Quick binary pattern scanner for CS:GO .so files"""
import struct
import sys
import os

def scan_pattern(data, pattern_str):
    """Scan binary data for a byte pattern with wildcards (?)"""
    parts = pattern_str.split(" ")
    pat_bytes = []
    wildcards = []
    for i, p in enumerate(parts):
        if p == "?":
            pat_bytes.append(0)
            wildcards.append(True)
        else:
            pat_bytes.append(int(p, 16))
            wildcards.append(False)
    
    results = []
    plen = len(pat_bytes)
    for i in range(len(data) - plen + 1):
        match = True
        for j in range(plen):
            if wildcards[j]:
                continue
            if data[i + j] != pat_bytes[j]:
                match = False
                break
        if match:
            results.append(i)
    return results

def gen_pattern(data, offset, length=32, max_wildcards=6):
    """Generate a pattern from bytes, replacing varying offsets with wildcards"""
    parts = []
    wildcard_count = 0
    prev = None
    for i in range(length):
        b = data[offset + i]
        # Try to detect relative offset fields (4-byte LE addresses that vary)
        if i < length - 3:
            val = struct.unpack_from("<I", data, offset + i)[0]
            # Likely a relative address if it looks like a small offset
            if 0 < val < 0x1000 and prev is not None and abs(val - prev) > 100:
                parts.append("?")
                wildcard_count += 1
                prev = val
                continue
            prev = val
        parts.append("%02X" % b)
        wildcard_count = 0
    return " ".join(parts)

# Patterns to search for
CLIENT_PATTERNS = {
    "renderBeams":       "4C 89 F6 4C 8B 25 ? ? ? ? 48 8D 05",
    "glowManager":       "E8 ? ? ? ? 48 8B 3D ? ? ? ? BE 01 00 00 00 C7",
    "initKeyValues":     "81 27 00 00 00 FF 55 31 C0 48 89 E5 5D",
    "loadFromBuffer":    "55 48 89 E5 41 57 41 56 41 55 41 54 49 89 D4 53 48 81 EC ? ? ? ? 48 85",
    "predictionSeed":    "48 8B 05 ? ? ? ? 8B 38 E8 ? ? ? ? 89 C7",
    "moveData":          "48 8B 0D ? ? ? ? 4C 89 EA",
    "restoreEntity":     "55 48 89 E5 41 57 41 89 D7 41 56 41 55 41 89 F5 41 54 53 48 83 EC 18",
    "lineGoesThruSmoke": "55 48 89 E5 41 56 41 55 41 54 53 48 83 EC 30 66 0F D6 45 D0",
}

ENGINE_PATTERNS = {
    "hostSecure":    "55 48 89 E5 E8 ? ? ? ? 48 8D 35 ? ? ? ? 48 8B 10 48 89 C7 FF 52 58 85 C0 74 13",
    "sendClantag":   "55 48 89 E5 41 55 49 89 FD 41 54 BF",
    "setNamedSkybox":"55 4C 8D 05 ? ? ? ? 48 89 E5 41",
}

def search_strings(data, strings):
    """Search for ASCII strings in binary"""
    results = {}
    for s in strings:
        positions = []
        encoded = s.encode("ascii")
        start = 0
        while True:
            pos = data.find(encoded, start)
            if pos == -1:
                break
            positions.append(pos)
            start = pos + 1
        if positions:
            results[s] = positions
    return results

def main():
    libs_dir = sys.argv[1] if len(sys.argv) > 1 else r"C:\Users\calloc\Desktop\libs"
    
    for fname, patterns in [("client_client.so", CLIENT_PATTERNS), ("engine_client.so", ENGINE_PATTERNS)]:
        fpath = os.path.join(libs_dir, fname)
        if not os.path.exists(fpath):
            print("ERROR: %s not found" % fpath)
            continue
        
        with open(fpath, "rb") as f:
            data = f.read()
        
        print("=" * 70)
        print("  %s  (%d bytes)" % (fname, len(data)))
        print("=" * 70)
        
        for name, pattern in patterns.items():
            results = scan_pattern(data, pattern)
            if results:
                print("[FOUND]   %-20s  matches: %s" % (name, ", ".join(["0x%X" % r for r in results])))
            else:
                print("[MISSING] %-20s" % name)
        
        print()
    
    # Search for useful strings in client_client.so to help find functions
    client_path = os.path.join(libs_dir, "client_client.so")
    if os.path.exists(client_path):
        with open(client_path, "rb") as f:
            data = f.read()
        
        print("=" * 70)
        print("  STRING SEARCH (client_client.so)")
        print("=" * 70)
        
        search_terms = [
            "RenderBeams", "DrawBeams", "render_beams",
            "CGlowObjectManager", "glow_manager", "glowobject",
            "KeyValues", "InitKeyValues", "LoadFromBuffer",
            "PredictionSeed", "prediction_seed", "pred_seed",
            "MoveData", "move_data", "CMoveData",
            "RestoreEntity", "restore_entity", "restoreToPredictedFrame",
            "LineGoesThroughSmoke", "line_goes_through_smoke",
            "smoke_grenade",
            "CNewParticleEffect",
            "DrawBeams",
            "m_pActiveParticles",
        ]
        
        found = search_strings(data, search_terms)
        for s, positions in sorted(found.items()):
            hex_positions = ["0x%X" % p for p in positions[:5]]
            extra = " ..." if len(positions) > 5 else ""
            print("  '%s' found at: %s%s" % (s, ", ".join(hex_positions), extra))
        
        print()
        
        engine_path = os.path.join(libs_dir, "engine_client.so")
        if os.path.exists(engine_path):
            with open(engine_path, "rb") as f:
                edata = f.read()
            
            print("=" * 70)
            print("  STRING SEARCH (engine_client.so)")
            print("=" * 70)
            
            engine_search = [
                "ClanTag", "clan_tag", "SetClanTag", "send_clan_tag",
                "LineGoesThroughSmoke",
                "Host_IsSecureServerAllowed",
            ]
            
            found = search_strings(edata, engine_search)
            for s, positions in sorted(found.items()):
                hex_positions = ["0x%X" % p for p in positions[:5]]
                extra = " ..." if len(positions) > 5 else ""
                print("  '%s' found at: %s%s" % (s, ", ".join(hex_positions), extra))

if __name__ == "__main__":
    main()
