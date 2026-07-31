#include "../../interfaces.hpp"
#include "../entity.hpp"

namespace Prediction {
    void startPrediction(CUserCmd* cmd) {
        inPrediction = true;
        if (!EntityCache::localPlayer)
            return;
        
        if (Interfaces::predictionSeed)
            *Interfaces::predictionSeed = rand() & 0x7FFFFFFF;

        oldCurtime = Interfaces::globals->curtime;
        oldFrametime = Interfaces::globals->frametime;

        Interfaces::globals->curtime = EntityCache::localPlayer->nDT_LocalPlayerExclusive__m_nTickBase() * Interfaces::globals->interval_per_tick;
        Interfaces::globals->frametime = Interfaces::globals->interval_per_tick;

        Interfaces::movement->startTrackPredictionErrors(EntityCache::localPlayer);

        if (Interfaces::moveHelper)
            Interfaces::moveHelper->setHost(EntityCache::localPlayer);
        if (Interfaces::moveData)
            Interfaces::prediction->setupMove(EntityCache::localPlayer, cmd, Interfaces::moveHelper, Interfaces::moveData);
        if (Interfaces::moveData)
            Interfaces::movement->processMovement(EntityCache::localPlayer, Interfaces::moveData);
        if (Interfaces::moveData)
            Interfaces::prediction->finishMove(EntityCache::localPlayer, cmd, Interfaces::moveData);
    }

    void endPrediction() {
        if (!EntityCache::localPlayer) {
            return;
        }

        Interfaces::movement->finishTrackPredictionErrors(EntityCache::localPlayer);
        if (Interfaces::moveHelper)
            Interfaces::moveHelper->setHost(0);

        if (Interfaces::predictionSeed)
            *Interfaces::predictionSeed = -1;

        Interfaces::globals->curtime = oldCurtime;
        Interfaces::globals->frametime = oldFrametime;
        inPrediction = false;
    }
}