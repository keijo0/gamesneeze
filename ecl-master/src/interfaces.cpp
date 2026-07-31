#include <cstdint>
#include <string>
#include <cstdio>

#include "interfaces.hpp"
#include "util/log.hpp"
#include "menu/config.hpp"

#define MARKER(msg) do { \
    FILE* _fp = std::fopen("/tmp/eclipse-injected", "a"); \
    if (_fp) { std::fprintf(_fp, "%s\n", msg); std::fflush(_fp); std::fclose(_fp); } \
} while(0)

template <typename T>
static constexpr auto relativeToAbsolute(std::uintptr_t address) noexcept {
    return (T)(address + 4 + *reinterpret_cast<std::int32_t*>(address));
}

bool host_IsSecureServerAllowed_hook() {
    return CONFIGBOOL("insecure bypass");
}

namespace Interfaces {
    void init() {
        MARKER("if-cvar");
        cvar = getInterface<ICvar>("./bin/linux64/materialsystem_client.so", "VEngineCvar");
        MARKER("if-loginit");
        Log::init();

        MARKER("if-engine");
        engine = getInterface<IVEngineClient>("./bin/linux64/engine_client.so", "VEngineClient");
        MARKER("if-client");
        client = getInterface<IBaseClientDLL>("./csgo/bin/linux64/client_client.so", "VClient");
        MARKER("if-entitylist");
        entityList = getInterface<IClientEntityList>("./csgo/bin/linux64/client_client.so", "VClientEntityList");
        MARKER("if-modelrender");
        modelRender = getInterface<IVModelRender>("./bin/linux64/engine_client.so", "VEngineModel");
        MARKER("if-modelinfo");
        modelInfo = getInterface<IVModelInfo>("./bin/linux64/engine_client.so", "VModelInfoClient");
        MARKER("if-materialsystem");
        materialSystem = getInterface<IMaterialSystem>("./bin/linux64/materialsystem_client.so", "VMaterialSystem");
        MARKER("if-studiorender");
        studioRender = getInterface<StudioRender>("./bin/linux64/studiorender_client.so", "VStudioRender");
        MARKER("if-panorama");
        panorama = getInterface<IPanoramaUIEngine>("./bin/linux64/panorama_gl_client.so", "PanoramaUIEngine");
        MARKER("if-sound");
        sound = getInterface<IEngineSound>("./bin/linux64/engine_client.so", "IEngineSoundClient");
        MARKER("if-events");
        eventManager = getInterface<IGameEventManager2>("./bin/linux64/engine_client.so", "GAMEEVENTSMANAGER002", true);
        MARKER("if-prediction");
        prediction = getInterface<IPrediction>("./csgo/bin/linux64/client_client.so", "VClientPrediction001", true);
        MARKER("if-movement");
	    movement = getInterface<IGameMovement>("./csgo/bin/linux64/client_client.so", "GameMovement");
        MARKER("if-trace");
	    trace = getInterface<IEngineTrace>("./bin/linux64/engine_client.so", "EngineTraceClient");
        MARKER("if-effects");
	    effects = getInterface<CEffects>("./bin/linux64/engine_client.so", "VEngineEffects");
        MARKER("if-filesystem");
	    fileSystem = getInterface<IFileSystem>("./bin/linux64/filesystem_stdio_client.so", "VFileSystem");
        MARKER("if-modelcache");
	    modelCache = getInterface<ModelCache>("./bin/linux64/datacache_client.so", "MDLCache");
        MARKER("if-stringtable");
        stringTableContainer = getInterface<CNetworkStringTableContainer>("./bin/linux64/engine_client.so", "VEngineClientStringTable");
        MARKER("if-clientmode");
        /* Get IClientMode */
        uintptr_t HudProcessInput = reinterpret_cast<uintptr_t>(Memory::getVTable(client)[10]);
        typedef IClientMode* (*GetClientMode)();
        GetClientMode getClientMode = reinterpret_cast<GetClientMode>(Memory::getAbsoluteAddress(HudProcessInput + 11, 1, 5));
        clientMode = getClientMode();
        LOG(" ClientMode %lx", (uintptr_t)clientMode);

        MARKER("if-globals");
        /* Get Globals */
        uintptr_t hudUpdate = reinterpret_cast<uintptr_t>(Memory::getVTable(client)[11]);
        globals = *reinterpret_cast<GlobalVars**>(Memory::getAbsoluteAddress(hudUpdate + 13, 3, 7));
        LOG(" Globals %lx", (uintptr_t)globals);

        MARKER("if-renderbeams");
        uintptr_t renderBeamsPat = Memory::patternScan("/client_client.so", "4C 89 F6 4C 8B 25 ? ? ? ? 48 8D 05");
        MARKER("if-renderbeams-pat");
        if (renderBeamsPat) {
            renderBeams = **Memory::relativeToAbsolute<ViewRenderBeams***>(renderBeamsPat + 6);
        }
        MARKER("if-renderbeams-done");

        MARKER("if-glowmanager");
        uintptr_t glowManagerPat = Memory::patternScan("/client_client.so",
                "E8 ? ? ? ? 48 8B 3D ? ? ? ? BE 01 00 00 00 C7");
        LOG(" glowManagerPat %lx", glowManagerPat);
        if (glowManagerPat)
            glowManager = *Memory::relativeToAbsolute<GlowObjectManager**>(glowManagerPat + 8);
        LOG(" glowManager %lx", glowManager);
        MARKER("if-glowmanager-done");

        MARKER("if-panelarray");
        MARKER("if-panelarray-done");

        MARKER("if-predseed");
        uintptr_t predSeedPat = Memory::patternScan("/client_client.so", 
                "48 8B 05 ? ? ? ? 8B 38 E8 ? ? ? ? 89 C7");
        LOG(" predSeedPat %lx", predSeedPat);
        if (predSeedPat)
            predictionSeed = *reinterpret_cast<int **>(Memory::getAbsoluteAddress(predSeedPat, 3, 7));
        LOG(" predictionSeed %lx", predictionSeed);

        MARKER("if-movehelper");
        uintptr_t moveHelperPat = Memory::patternScan("/client_client.so", 
                "00 48 89 3D ? ? ? ? C3");
        LOG(" moveHelperPat %lx", moveHelperPat);
        if (moveHelperPat)
            moveHelper = *reinterpret_cast<IMoveHelper **>(Memory::getAbsoluteAddress(moveHelperPat + 1, 3, 7));
        LOG(" moveHelper %lx", moveHelper);

        MARKER("if-movedata");
        uintptr_t moveDataPat = Memory::patternScan("/client_client.so", 
                "48 8B 0D ? ? ? ? 4C 89 EA");
        LOG(" moveDataPat %lx", moveDataPat);
        if (moveDataPat)
            moveData = **reinterpret_cast<CMoveData***>(Memory::getAbsoluteAddress(moveDataPat, 3, 7));
        LOG(" moveData %lx", moveData);

        MARKER("if-restoreentity");
        restoreEntityToPredictedFrame = (RestoreEntityToPredictedFrame)Memory::patternScan("/client_client.so",
            "55 48 89 E5 53 31 DB 48 83 EC 08 EB 2E 0F 1F 00 31 FF E8");
        LOG(" restoreEntityToPredictedFrame %lx", restoreEntityToPredictedFrame);

        MARKER("if-smoke");
        lineGoesThroughSmoke = (LineGoesThroughSmoke)Memory::patternScan("/client_client.so", 
                "55 48 89 E5 41 56 41 55 41 54 53 48 83 EC 30 8B 05 ? ? ? ? 66 0F D6 45 D0 F3 0F 11 4D D8 66 0F D6 55 C0 F3 0F 11 5D");
        LOG(" lineGoesThroughSmoke | %lx", lineGoesThroughSmoke);

        MARKER("if-hostsecure");
        host_IsSecureServerAllowed = (Host_IsSecureServerAllowed)Memory::patternScan("/engine_client.so", "55 48 89 E5 E8 ? ? ? ? 48 8D 35 ? ? ? ? 48 8B 10 48 89 C7 FF 52 58 85 C0 74 13");
        if (host_IsSecureServerAllowed) {
            insecure = !host_IsSecureServerAllowed();
            if (insecure) {
                Memory::VMT::detour((char*)(host_IsSecureServerAllowed), (char*)(host_IsSecureServerAllowed_hook));
            }
        }

        MARKER("if-done");
        LOG("Initialised interfaces!");
    }

    void testAllPatterns() {
        MARKER("=== PATTERN SCAN RESULTS ===");
        auto testPat = [](const char* label, const char* module, const char* pattern) {
            uintptr_t result = Memory::patternScan(module, pattern);
            char buf[512];
            if (result) {
                snprintf(buf, sizeof(buf), "[FOUND]   %s (%s) -> %lx", label, module, result);
            } else {
                snprintf(buf, sizeof(buf), "[MISSING] %s (%s)", label, module);
            }
            MARKER(buf);
        };

        testPat("renderBeams",         "/client_client.so", "4C 89 F6 4C 8B 25 ? ? ? ? 48 8D 05");
        testPat("glowManager",         "/client_client.so", "E8 ? ? ? ? 48 8B 3D ? ? ? ? BE 01 00 00 00 C7");
        testPat("initKeyValues",       "/client_client.so", "81 27 00 00 00 FF 55 31 C0 48 89 E5 5D");
        testPat("loadFromBuffer",      "/client_client.so", "55 48 89 E5 41 57 41 56 41 55 41 54 53 48 83 EC 18 48 85");
        testPat("predictionSeed",      "/client_client.so", "48 8B 05 ? ? ? ? 8B 38 E8 ? ? ? ? 89 C7");
        testPat("moveHelper",          "/client_client.so", "00 48 89 3D ? ? ? ? C3");
        testPat("moveData",            "/client_client.so", "48 8B 0D ? ? ? ? 4C 89 EA");
        testPat("restoreEntity",       "/client_client.so", "55 48 89 E5 53 31 DB 48 83 EC 08 EB 2E 0F 1F 00 31 FF E8");
        testPat("lineGoesThruSmoke",   "/client_client.so", "55 48 89 E5 41 56 41 55 41 54 53 48 83 EC 30 8B 05 ? ? ? ? 66 0F D6 45 D0 F3 0F 11 4D D8 66 0F D6 55 C0 F3 0F 11 5D");
        testPat("hostSecure",          "/engine_client.so", "55 48 89 E5 E8 ? ? ? ? 48 8D 35 ? ? ? ? 48 8B 10 48 89 C7 FF 52 58 85 C0 74 13");
        testPat("sendClantag",         "/engine_client.so", "55 48 89 E5 41 55 49 89 FD 41 54 BF");
        testPat("setNamedSkybox",      "/engine_client.so", "55 4C 8D 05 ? ? ? ? 48 89 E5 41");

        MARKER("=== END PATTERN SCAN ===");
    }
}