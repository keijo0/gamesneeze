#include "../includes.hpp"
#include <mutex>
#include <cstdio>

/* print Logs in green */
std::mutex logLock;

void writeMarker(const char* message) {
    FILE* fp = std::fopen("/tmp/gamesneeze-injected", "a");
    if (fp) {
        std::fprintf(fp, "%s\n", message);
        std::fflush(fp);
        std::fclose(fp);
    }
}

void Log::log(logLevel level, const char* fmt, ...) {
    std::lock_guard<std::mutex> lock(logLock);
    
    va_list args;
    va_start(args, fmt);
    char buf[5000];
    vsnprintf(buf, sizeof(buf), fmt, args);

    switch (level) {
        case LOG: {
            fputs("\e[32m[LOG] ", stdout); 
            if (Interfaces::convar) {
                Interfaces::convar->ConsoleColorPrintf({0, 255, 0, 255}, "[LOG] %s\n", buf);
            }
            break;
        }
        case WARN: {
            fputs("\e[33m[WARN] ", stdout); 
            if (Interfaces::convar) {
                Interfaces::convar->ConsoleColorPrintf({255, 255, 0, 255}, "[WARN] %s\n", buf);
            }
            break;
        }
        case ERR: {
            fputs("\e[31m[ERR] ", stdout); 
            if (Interfaces::convar) {
                Interfaces::convar->ConsoleColorPrintf({255, 0, 0, 255}, "[ERR] %s\n", buf);
            }
            break;
        }
    }
    fputs(buf, stdout); 
    fputs("\e[0m\n", stdout);
    va_end(args);
}