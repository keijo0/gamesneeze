#include "hooks.hpp"
#include "util/log.hpp"
#include "interfaces.hpp"
#include "features/chams.hpp"
#include "sdk/netvars.hpp"
#include "features/discordrpc.hpp"
#include "util/protection/protection.hpp"

#include <dlfcn.h>
#include <thread>
#include <string>
#include <cstdlib>
#include <unistd.h>
#include <cstdio>
#include <csignal>
#include <atomic>

static std::atomic<bool> g_initStarted{false};

bool hk() {
    return true;
}

namespace {
    bool shouldUseLegacyInit() {
        return true;
    }

    void writeMarker(const char* message) {
        FILE* fp = std::fopen("/tmp/eclipse-injected", "a");
        if (fp) {
            std::fprintf(fp, "%s\n", message);
            std::fflush(fp);
            std::fclose(fp);
        }
    }
}

void mainThread() {
    writeMarker("eclipse-injected");
    std::fprintf(stderr, "[eclipse] injected stub startup active\n");
    LOG("eclipse injected: startup stub active");

    if (!shouldUseLegacyInit()) {
        writeMarker("eclipse-safe-startup");
        std::fprintf(stderr, "[eclipse] safe startup enabled; no legacy hooks initialized\n");
        LOG("Safe startup enabled; no legacy hooks will be initialized.");
        return;
    }

    writeMarker("step-interfaces");
    Interfaces::init();
    writeMarker("step-patterntest");
    Interfaces::testAllPatterns();
    writeMarker("step-protection");
    Protection::protect();
    writeMarker("step-chams");
    Chams::createMaterials();
    writeMarker("step-netvars");
    Netvars::init();
    writeMarker("step-hooks");
    Hooks::init();
    writeMarker("step-done");

    LOG("Successfully loaded eclipse!");
}

/* Called on uninject, if you ld_preload with this, then it will call it as soon as you inject, so only have this if PRELOAD compile def is not set */
#ifndef PRELOAD
void __attribute__((destructor)) unload() {
    LOG("Unloading eclipse...");
    Hooks::unload();
    LOG("Unloaded eclipse!");
    if (DiscordRPC::core) {
        DiscordRPC::core->~Core();
        DiscordRPC::core = nullptr;
    }
}
#endif

/* Called when injected */
extern "C" __attribute__((visibility("default"))) __attribute__((used)) __attribute__((externally_visible)) void eclipse_init() {
    std::thread thread(mainThread);
    thread.detach();
}
