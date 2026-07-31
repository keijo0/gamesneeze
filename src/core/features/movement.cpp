#include "../../includes.hpp"
#include "features.hpp"

namespace {
    bool should_longjump = false;
    bool should_minijump = false;
    bool should_pixelsurf = false;
    static int pixelsurf_ticks = 0;
    bool Awall = false;
    float flZVelBackup = 0.f;
    float flBugSpeed = 0.f;
    bool jumpBugAGFix = false;
    static float startCircle = 0.f;
    static Vector wallPosition;

    struct PointCheck {
        Vector Pos;
        std::string Map;
    };
    static std::vector<PointCheck> PointsCheck;
    static bool seccheckcros = false;
    static bool HITGODA = false;
    static bool togglechecksurflolkek = false;
    static bool HitJump = false;
    static bool HitMiniJump = false;
    static bool HitLongJump = false;
    static bool HitCHop = false;
    static bool HitJumpBug = false;
    static bool ChatStand = false;
    static bool ChatDuck = false;
    static Vector ChatPositions;

    bool check(float a, float b) {
        int a1 = (int)a;
        int b1 = (int)b;
        if (b < 0) {
            if (a1 == b1) {
                int a2 = (int)((a - a1) * 100);
                int b2 = (int)((b - b1) * 100);
                if (b2 == a2 || a2 + 1 == b2 || a2 + 2 == b2)
                    return true;
                else
                    return false;
            }
        } else {
            if (a1 == b1) {
                int a2 = (int)((a - a1) * 100);
                int b2 = (int)((b - b1) * 100);
                if (b2 == a2 || a2 == b2 - 1 || a2 == b2 - 2)
                    return true;
                else
                    return false;
            }
        }
        return false;
    }
}

void bhop(CUserCmd *cmd) {
    if (CONFIGBOOL("Misc>Misc>Movement>JumpBug") &&
        Menu::CustomWidgets::isKeyDown(CONFIGINT("Misc>Misc>Movement>JumpBug Key")))
        return;
    if (CONFIGBOOL("Misc>Misc>Movement>Auto Hop")) {
        if (Globals::localPlayer->moveType() == 9)
            return;
        if (CONFIGBOOL("Misc>Misc>Movement>Humanised Bhop")) {
            // https://www.unknowncheats.me/forum/counterstrike-global-offensive/333797-humanised-bhop.html
            static int hopsRestricted = 0;
            static int hopsHit = 0;
            if (!(Globals::localPlayer->flags() & FL_ONGROUND)) {
                cmd->buttons &= ~IN_JUMP;
                hopsRestricted = 0;
            } else if ((rand() % 100 > CONFIGINT("Misc>Misc>Movement>Bhop Hitchance") &&
                        hopsRestricted <
                           CONFIGINT("Misc>Misc>Movement>Bhop Max Misses")) ||
                       (CONFIGINT("Misc>Misc>Movement>Bhop Max Hops Hit") > 0 &&
                        hopsHit > CONFIGINT("Misc>Misc>Movement>Bhop Max Hops Hit"))) {
                cmd->buttons &= ~IN_JUMP;
                hopsRestricted++;
                hopsHit = 0;
            } else {
                hopsHit++;
            }
        } else {
            if (!(Globals::localPlayer->flags() & FL_ONGROUND)) {
                cmd->buttons &= ~IN_JUMP;
            }
        }
    }
}

void edgeJump(CUserCmd *cmd) {
    if (CONFIGBOOL("Misc>Misc>Movement>Edge Jump") &&
        Menu::CustomWidgets::isKeyDown(CONFIGINT("Misc>Misc>Movement>Edge Jump Key")) &&
        Features::Movement::flagsBackup & FL_ONGROUND &&
        !(Globals::localPlayer->flags() & FL_ONGROUND))
        cmd->buttons |= IN_JUMP;
}

void jumpBug(CUserCmd *cmd) {
    static bool jumpbug_triggered = false;
    static bool crouched = false;
    
    if (!CONFIGBOOL("Misc>Misc>Movement>JumpBug") ||
        !Menu::CustomWidgets::isKeyDown(CONFIGINT("Misc>Misc>Movement>JumpBug Key"))) {
        jumpbug_triggered = false;
        crouched = false;
        return;
    }

    if (!Globals::localPlayer || !Globals::localPlayer->health())
        return;

    if (Globals::localPlayer->moveType() == MOVETYPE_NOCLIP ||
        Globals::localPlayer->moveType() == MOVETYPE_LADDER ||
        Globals::localPlayer->moveType() == MOVETYPE_OBSERVER) {
        jumpbug_triggered = false;
        crouched = false;
        return;
    }

    // Experimental jump bug logic from lobotomy
    // Duck immediately when transitioning from air to ground
    if (!(Features::Movement::flagsBackup & FL_ONGROUND) &&
        (Globals::localPlayer->flags() & FL_ONGROUND)) {
        cmd->buttons |= IN_DUCK;
        cmd->buttons &= ~IN_JUMP;
    }

    // Check if we should trigger jump bug (velocity going up after falling)
    bool should = (!(Features::Movement::flagsBackup & FL_ONGROUND) &&
                   !(Globals::localPlayer->flags() & FL_ONGROUND) &&
                   (Globals::localPlayer->velocity().z > Features::Movement::velBackup.z) &&
                   (Globals::localPlayer->moveType() != MOVETYPE_NOCLIP &&
                    Globals::localPlayer->moveType() != MOVETYPE_LADDER &&
                    Globals::localPlayer->moveType() != MOVETYPE_OBSERVER));

    if (!should) {
        if (!jumpbug_triggered) {
            if (!(Features::Movement::flagsBackup & FL_ONGROUND) &&
                (Globals::localPlayer->flags() & FL_ONGROUND)) {
                if (cmd->buttons & IN_DUCK) {
                    if (Globals::localPlayer->flags() & FL_DUCKING) {
                        cmd->buttons &= ~IN_DUCK;
                        crouched = true;
                    } else {
                        cmd->buttons |= IN_DUCK;
                    }
                }
                cmd->buttons &= ~IN_JUMP;
                jumpbug_triggered = true;
            }
        } else {
            if (crouched) {
                cmd->buttons &= ~IN_DUCK;
                crouched = false;
            } else {
                cmd->buttons |= IN_DUCK;
                jumpbug_triggered = false;
            }
        }
    }
}

void longJump(CUserCmd *cmd) {
    static float btime;
    if (btime < Interfaces::globals->curtime)
        should_longjump = false;
    if (!Globals::localPlayer)
        btime = 0.f;

    if (!CONFIGBOOL("Misc>Misc>Movement>LongJump") ||
        !Menu::CustomWidgets::isKeyDown(CONFIGINT("Misc>Misc>Movement>LongJump Key")))
        return;

    if (!CONFIGBOOL("Misc>Misc>Movement>Edge Jump"))
        return;

    if (!Globals::localPlayer)
        return;

    const auto move_type = Globals::localPlayer->moveType();

    if (move_type == MOVETYPE_LADDER || move_type == MOVETYPE_NOCLIP)
        return;

    static int saved_tick_count;

    if (Globals::localPlayer->flags() & FL_ONGROUND) {
        saved_tick_count = Interfaces::globals->tickcount;
    }

    bool do_lj;
    if (Interfaces::globals->tickcount - saved_tick_count > 2) {
        do_lj = false;
    } else {
        do_lj = true;
    }

    if (do_lj && !(Globals::localPlayer->flags() & FL_ONGROUND)) {
        cmd->buttons |= IN_DUCK;
        btime = Interfaces::globals->curtime + 0.2f;
        should_longjump = true;
    }
}

void miniJump(CUserCmd *cmd) {
    static float btime;
    if (btime < Interfaces::globals->curtime)
        should_minijump = false;
    if (!Globals::localPlayer)
        btime = 0.f;

    if (!CONFIGBOOL("Misc>Misc>Movement>MiniJump") ||
        !Menu::CustomWidgets::isKeyDown(CONFIGINT("Misc>Misc>Movement>MiniJump Key")))
        return;

    if (!Globals::localPlayer)
        return;

    if (Globals::localPlayer->moveType() == MOVETYPE_LADDER ||
        Globals::localPlayer->moveType() == MOVETYPE_NOCLIP)
        return;

    static int minijump_tick = 0;
    static bool minijumpbool = false;

    if ((Features::Movement::flagsBackup & FL_ONGROUND) && !(Globals::localPlayer->flags() & FL_ONGROUND)) {
        cmd->buttons |= IN_JUMP;
        minijumpbool = true;
        btime = Interfaces::globals->curtime + 0.2f;
        should_minijump = true;
        minijump_tick = cmd->tick_count + 1;
        cmd->buttons |= IN_DUCK;
    }
}

void pixelSurf(CUserCmd *cmd) {
    if (!CONFIGBOOL("Misc>Misc>Movement>PixelSurf"))
        return;

    if (!Globals::localPlayer || !Globals::localPlayer->health())
        return;

    if (Globals::localPlayer->moveType() == MOVETYPE_LADDER ||
        Globals::localPlayer->moveType() == MOVETYPE_NOCLIP)
        return;

    if (Globals::localPlayer->flags() & FL_ONGROUND)
        return;

    if (Menu::CustomWidgets::isKeyDown(CONFIGINT("Misc>Misc>Movement>EdgeBug Key")))
        return;

    if (!Features::Prediction::inPrediction)
        return;

    // Collideable check like lobotomy
    auto collideable = &Globals::localPlayer->collideable();
    if (!collideable)
        return;

    // Wall detection - must run before prediction logic
    if (Globals::localPlayer->velocity().z != -6.25f && flZVelBackup != -6.25f)
        startCircle = 0.f;

    Awall = false;
    Trace trace;
    float step = (float)M_PI * 2.0f / 16.f;
    Vector wallPosition;

    for (float a = startCircle; a < (M_PI * 2.0f); a += step) {
        Vector wishdir = Vector(cosf(a), sinf(a), 0.f);
        const auto startPos = Globals::localPlayer->abs_origin();
        const auto endPos = startPos + wishdir;

        CTraceFilterWorldOnly filter;
        Ray ray;
        ray.Init(startPos, endPos, Globals::localPlayer->collideable().OBBMins(), Globals::localPlayer->collideable().OBBMaxs());
        Interfaces::trace->TraceRay(ray, MASK_PLAYERSOLID, &filter, &trace);

        if ((trace.fraction < 1.f) && (trace.plane.normal.z == 0.f)) {
            wallPosition = trace.endpos;
            startCircle = a;
            Awall = true;
            break;
        }
    }

    if (!Awall)
        return;

    if (!should_pixelsurf) {
        int nCommandsPredicted = Interfaces::prediction->Split[0].nCommandsPredicted;
        int BackupButtons = cmd->buttons;

        for (int i = 0; i < 2; i++) {
            Features::Prediction::restoreEntityToPredictedFrame(nCommandsPredicted - 1);
            if (i == 0)
                cmd->buttons &= ~IN_DUCK;
            else
                cmd->buttons |= IN_DUCK;

            for (int z = 0; z < 8; z++) {
                Features::Prediction::start(cmd);
                Features::Prediction::end();

                if (Globals::localPlayer->flags() & FL_ONGROUND)
                    break;

                float zVelo = Globals::localPlayer->velocity().z;
                should_pixelsurf = flZVelBackup < 10.f && zVelo == -6.25f;

                if (should_pixelsurf && i == 0) {
                    should_pixelsurf = false;
                    cmd->buttons = BackupButtons;
                    return;
                }

                if (should_pixelsurf) {
                    pixelsurf_ticks = cmd->tick_count + z + 16;
                    BackupButtons = cmd->buttons;
                    break;
                }
            }
        }
        cmd->buttons = BackupButtons;
        Features::Prediction::restoreEntityToPredictedFrame(nCommandsPredicted);
    } else {
        cmd->buttons |= IN_DUCK;
        if (cmd->tick_count > pixelsurf_ticks) {
            if (flZVelBackup != -6.25f) {
                should_pixelsurf = false;
            }
        }
    }
}

void autoAlign(CUserCmd *cmd) {
    if (!CONFIGBOOL("Misc>Misc>Movement>AutoAlign"))
        return;

    if (!Globals::localPlayer)
        return;

    if (Features::Movement::flagsBackup & FL_ONGROUND) {
        jumpBugAGFix = false;
        return;
    }

    if (Globals::localPlayer->moveType() == MOVETYPE_LADDER || 
        Globals::localPlayer->moveType() == MOVETYPE_NOCLIP)
        return;

    if (Globals::localPlayer->velocity().z > 0)
        jumpBugAGFix = false;

    if (jumpBugAGFix)
        return;

    if (Menu::CustomWidgets::isKeyDown(CONFIGINT("Misc>Misc>Movement>FireMan Key")))
        return;

    // Collideable check like lobotomy
    auto collideable = &Globals::localPlayer->collideable();
    if (!collideable)
        return;

    // Wall detection - always run like lobotomy
    Trace trace;
    float step = (float)M_PI * 2.0f / 16.f;
    Awall = false;

    if (Globals::localPlayer->velocity().z != -6.25f && flZVelBackup != -6.25f)
        startCircle = 0.f;

    Vector wallPosition{};
    for (float a = startCircle; a < (M_PI * 2.0f); a += step) {
        Vector wishdir = Vector(cosf(a), sinf(a), 0.f);
        const auto startPos = Globals::localPlayer->abs_origin();
        const auto endPos = startPos + wishdir;

        CTraceFilterWorldOnly filter;
        Ray ray;
        ray.Init(startPos, endPos, Globals::localPlayer->collideable().OBBMins(), Globals::localPlayer->collideable().OBBMaxs());
        Interfaces::trace->TraceRay(ray, MASK_PLAYERSOLID, &filter, &trace);

        if ((trace.fraction < 1.f) && (trace.plane.normal.z == 0.f)) {
            wallPosition = trace.endpos;
            startCircle = a;
            Awall = true;
            break;
        }
    }

    if (!Awall)
        return;

    if (Awall && !Menu::CustomWidgets::isKeyDown(CONFIGINT("Misc>Misc>Movement>Airstuck Key"))) {
        int nCommandsPredicted = Interfaces::prediction->Split[0].nCommandsPredicted;
        Vector normalPlane = Vector(trace.plane.normal.x * -1.f, trace.plane.normal.y * -1.f, 0.f);
        QAngle wallAngle = toAngle(normalPlane);

        if (Globals::localPlayer->velocity().Length2D() > 280 && !(cmd->buttons & IN_DUCK)) {
            if (Globals::localPlayer->velocity().z == -6.25f || flZVelBackup == -6.25f) {
                float mVel = hypotf(Globals::localPlayer->velocity().x, Globals::localPlayer->velocity().y);
                float ideal = Convert2(atanf(25.f / mVel));
                Vector dvelo = Globals::localPlayer->velocity();
                dvelo.z = 0.f;
                QAngle velo_angle = toAngle(dvelo);
                QAngle delta = velo_angle - wallAngle;
                normalizeAngles(delta);
                if (delta.y >= 0.f)
                    wallAngle.y += ideal;
                else
                    wallAngle.y -= ideal;
            }
        }

        float rotation = Convert(wallAngle.y - cmd->viewangles.y);
        float cos_rot = cos(rotation);
        float sin_rot = sin(rotation);
        float multiplayer = 0.f;

        if (Globals::localPlayer->velocity().z == -6.25f || flZVelBackup == -6.25f)
            multiplayer = 45.f;
        else
            multiplayer = 6.f;

        float forwardmove = cos_rot * multiplayer;
        float sidemove = -sin_rot * multiplayer;

        Trace secTrace;
        Ray ray;
        TraceFilter filter;
        filter.pSkip = Globals::localPlayer;
        Vector startPos = Vector(Globals::localPlayer->origin().x, Globals::localPlayer->origin().y, wallPosition.z);
        Vector endPos = startPos + normalPlane * 64.f;
        ray.Init(Globals::localPlayer->origin(), endPos);
        Interfaces::trace->TraceRay(ray, MASK_PLAYERSOLID, &filter, &secTrace);

        if (secTrace.fraction > 0.249715533f || 
            !(cmd->buttons & IN_FORWARD) && !(cmd->buttons & IN_BACK) && 
            !(cmd->buttons & IN_MOVELEFT) && !(cmd->buttons & IN_MOVERIGHT)) {
            cmd->forwardmove = forwardmove;
            cmd->sidemove = sidemove;
        }

        if (Globals::localPlayer->velocity().Length2D() > 280 && !(cmd->buttons & IN_DUCK)) {
            if (Globals::localPlayer->velocity().z == -6.25f || flZVelBackup == -6.25f) {
                if (cmd->forwardmove < 0.f && cmd->buttons & IN_FORWARD)
                    cmd->forwardmove = 450.f;
                if (cmd->forwardmove > 0.f && cmd->buttons & IN_BACK)
                    cmd->forwardmove = -450.f;
                if (cmd->sidemove < 0.f && cmd->buttons & IN_MOVERIGHT)
                    cmd->sidemove = 450.f;
                if (cmd->sidemove > 0.f && cmd->buttons & IN_MOVELEFT)
                    cmd->sidemove = -450.f;
                return;
            }
        }

        if (multiplayer == 45.f && (cmd->buttons & IN_FORWARD) || (cmd->buttons & IN_BACK) || 
            (cmd->buttons & IN_MOVELEFT) || (cmd->buttons & IN_MOVERIGHT)) {
            int buttons = cmd->buttons;
            float savedForwardmove = cmd->forwardmove;
            float savedSidemove = cmd->sidemove;

            for (int i = 0; i < 450; i += 45) {
                Features::Prediction::restoreEntityToPredictedFrame(nCommandsPredicted - 1);
                if (buttons & IN_FORWARD)
                    cmd->forwardmove = i;
                if (buttons & IN_BACK)
                    cmd->forwardmove = -i;
                if (buttons & IN_MOVELEFT)
                    cmd->sidemove = -i;
                if (buttons & IN_MOVERIGHT)
                    cmd->sidemove = i;

                Features::Prediction::start(cmd);
                Features::Prediction::end();
                float zvelo = Globals::localPlayer->velocity().z;
                if (zvelo == -flBugSpeed) {
                    savedForwardmove = cmd->forwardmove;
                    savedSidemove = cmd->sidemove;
                }
            }
            cmd->forwardmove = savedForwardmove;
            cmd->sidemove = savedSidemove;
        }

        if (cmd->forwardmove < 0.f && cmd->buttons & IN_FORWARD)
            cmd->forwardmove = 450.f;
        if (cmd->forwardmove > 0.f && cmd->buttons & IN_BACK)
            cmd->forwardmove = -450.f;
        if (cmd->sidemove < 0.f && cmd->buttons & IN_MOVERIGHT)
            cmd->sidemove = 450.f;
        if (cmd->sidemove > 0.f && cmd->buttons & IN_MOVELEFT)
            cmd->sidemove = -450.f;
    }

    static bool hit = false;
    if (Awall && Menu::CustomWidgets::isKeyDown(CONFIGINT("Misc>Misc>Movement>Airstuck Key")) && 
        CONFIGBOOL("Misc>Misc>Movement>Airstuck")) {
        int nCommandsPredicted = Interfaces::prediction->Split[0].nCommandsPredicted;
        Vector normalPlane = Vector(trace.plane.normal.x * -1.f, trace.plane.normal.y * -1.f, 0.f);
        QAngle wallAngle = toAngle(normalPlane);

        static float foundedForwardmove{};
        static float foundedSidemove{};
        static QAngle foundedViewAngle{};

        if (hit) {
            cmd->viewangles = foundedViewAngle;
            cmd->forwardmove = foundedForwardmove;
            cmd->sidemove = foundedSidemove;
        }

        if (Globals::localPlayer->velocity().z < -40.f || Globals::localPlayer->velocity().z > 0.f)
            hit = false;

        if (!hit) {
            float rotation = Convert(wallAngle.y - cmd->viewangles.y);
            float cos_rot = cos(rotation);
            float sin_rot = sin(rotation);
            float multiplayer = 45.f;

            float forwardmove = cos_rot * multiplayer;
            float sidemove = -sin_rot * multiplayer;

            int buttons = cmd->buttons;
            float savedForwardmove = cmd->forwardmove;
            float savedSidemove = cmd->sidemove;

            for (int i = 0; i < 450; i += 45) {
                Features::Prediction::restoreEntityToPredictedFrame(nCommandsPredicted - 1);
                if (buttons & IN_FORWARD)
                    cmd->forwardmove = i;
                if (buttons & IN_BACK)
                    cmd->forwardmove = -i;
                if (buttons & IN_MOVELEFT)
                    cmd->sidemove = -i;
                if (buttons & IN_MOVERIGHT)
                    cmd->sidemove = i;

                Features::Prediction::start(cmd);
                Features::Prediction::end();
                float zvelo = Globals::localPlayer->velocity().z;
                if (zvelo == -flBugSpeed) {
                    foundedForwardmove = cmd->forwardmove;
                    foundedSidemove = cmd->sidemove;
                    foundedViewAngle = cmd->viewangles;
                    hit = true;
                }
            }
            cmd->forwardmove = savedForwardmove;
            cmd->sidemove = savedSidemove;
        }
    }
}

void fireMan(CUserCmd *cmd) {
    if (!CONFIGBOOL("Misc>Misc>Movement>FireMan"))
        return;

    if (!Globals::localPlayer)
        return;

    if (!Globals::localPlayer->health())
        return;

    if (!Menu::CustomWidgets::isKeyDown(CONFIGINT("Misc>Misc>Movement>FireMan Key")))
        return;

    if (Globals::localPlayer->moveType() == MOVETYPE_LADDER) {
        cmd->buttons |= IN_JUMP;
        cmd->forwardmove = 0.f;
        cmd->sidemove = 0.f;
    }
}

void autoBounce(CUserCmd *cmd) {
    if (!CONFIGBOOL("Misc>Misc>Movement>AutoBounce"))
        return;

    if (!Globals::localPlayer)
        return;

    if (!Globals::localPlayer->health())
        return;

    if (Globals::localPlayer->moveType() == MOVETYPE_NOCLIP || 
        Globals::localPlayer->moveType() == MOVETYPE_OBSERVER || 
        Globals::localPlayer->moveType() == MOVETYPE_LADDER)
        return;

    if (Features::Movement::flagsBackup & FL_ONGROUND || Features::Movement::flagsBackup & FL_DUCKING)
        return;

    if (should_pixelsurf)
        return;

    if (Menu::CustomWidgets::isKeyDown(CONFIGINT("Misc>Misc>Movement>EdgeBug Key")) ||
        Menu::CustomWidgets::isKeyDown(CONFIGINT("Misc>Misc>Movement>JumpBug Key")) ||
        Menu::CustomWidgets::isKeyDown(CONFIGINT("Misc>Misc>Movement>AutoBounce Key")))
        return;

    float timepredicted = 0.f;
    float savedzpos = -999999.f;
    bool foundground = false;
    CUserCmd savedcmd = *cmd;

    Features::Prediction::restoreEntityToPredictedFrame(Interfaces::prediction->Split[0].nCommandsPredicted - 1);

    while (timepredicted < 0.05f && !(Globals::localPlayer->flags() & FL_ONGROUND)) {
        cmd->buttons |= IN_DUCK;
        Features::Prediction::start(cmd);
        Features::Prediction::end();
        if (Globals::localPlayer->flags() & FL_ONGROUND) {
            foundground = true;
            savedzpos = Globals::localPlayer->origin().z;
            break;
        } else {
            timepredicted += Interfaces::globals->interval_per_tick;
        }
    }

    bool foundgstanding = false;
    float savedzposstanding = -999999.f;

    if (foundground && savedzpos != -999999.f) {
        *cmd = savedcmd;
        Features::Prediction::restoreEntityToPredictedFrame(Interfaces::prediction->Split[0].nCommandsPredicted - 1);

        timepredicted = 0.f;

        while (timepredicted < 0.05f && !(Globals::localPlayer->flags() & FL_ONGROUND)) {
            cmd->buttons &= ~IN_DUCK;
            Features::Prediction::start(cmd);
            Features::Prediction::end();
            if (Globals::localPlayer->flags() & FL_ONGROUND) {
                foundgstanding = true;
                savedzposstanding = Globals::localPlayer->origin().z;
                break;
            } else {
                timepredicted += Interfaces::globals->interval_per_tick;
            }
        }
    }

    if (foundground && foundgstanding) {
        if (savedzpos > savedzposstanding) {
            cmd->buttons |= IN_DUCK;
        }
    } else if (foundground && !foundgstanding) {
        cmd->buttons |= IN_DUCK;
    } else {
        *cmd = savedcmd;
    }
}

void checkSurf(CUserCmd *cmd) {
    if (!CONFIGBOOL("Misc>Misc>Movement>CheckSurf"))
        return;

    if (!Globals::localPlayer)
        return;

    if (!Globals::localPlayer->health())
        return;

    static int toggletime = 0;

    if (Menu::CustomWidgets::isKeyPressed(CONFIGINT("Misc>Misc>Movement>CheckSurf Point Key")) && !seccheckcros) {
        QAngle viewangle = QAngle(cmd->viewangles.x, cmd->viewangles.y, 0.f);
        Vector direction;
        angleVectors(viewangle, direction);
        const auto endPos = Globals::localPlayer->eyePos() + direction * 2000.f;

        Trace trace;
        TraceFilter filter;
        filter.pSkip = Globals::localPlayer;
        Ray ray;
        ray.Init(Globals::localPlayer->eyePos(), endPos);
        Interfaces::trace->TraceRay(ray, MASK_PLAYERSOLID, &filter, &trace);

        if (trace.fraction == 1.f) {
            return;
        }

        PointCheck point;
        point.Pos = Globals::localPlayer->eyePos() + (endPos - Globals::localPlayer->eyePos()) * trace.fraction;
        point.Map = "";
        PointsCheck.push_back(point);
    }

    if (CONFIGBOOL("Misc>Misc>Movement>CheckSurf")) {
        if (Menu::CustomWidgets::isKeyDown(CONFIGINT("Misc>Misc>Movement>CheckSurf Key")) && toggletime < Interfaces::globals->tickcount) {
            toggletime = Interfaces::globals->tickcount + 30;
            togglechecksurflolkek = !togglechecksurflolkek;
        }
    }

    if (!Menu::CustomWidgets::isKeyDown(CONFIGINT("Misc>Misc>Movement>CheckSurf Key")) && !togglechecksurflolkek) {
        HITGODA = false;
        HitJump = false;
        HitMiniJump = false;
        HitLongJump = false;
        HitCHop = false;
        HitJumpBug = false;
        return;
    }

    if (PointsCheck.empty())
        return;

    int index = 0;
    float Nearest = 99999.f;
    for (size_t i = 0; i < PointsCheck.size(); i++) {
        float dist = Globals::localPlayer->origin().DistTo(PointsCheck.at(i).Pos);
        if (Nearest > dist) {
            Nearest = dist;
            index = i;
        }
    }

    Vector Surf = PointsCheck.at(index).Pos;
    Vector Calculate;
    float iCalcilate = 0.f;

    int BackupButtons = cmd->buttons;
    float ForwaMove = cmd->forwardmove;
    float SideMove = cmd->sidemove;

    int BackupPredicted = Interfaces::prediction->Split[0].nCommandsPredicted;
    static int ticks = 0;
    static int ljticks = 0;
    int g = (Features::Movement::flagsBackup & 1) ? 3 : 5;
    int hitgroudtick = 0;

    if (flZVelBackup < 0.f)
        ljticks = 0;

    for (int v = 0; v < g; v++) {
        cmd->sidemove = 0.f;
        cmd->forwardmove = 0.f;
        cmd->buttons = BackupButtons;

        Features::Prediction::restoreEntityToPredictedFrame(BackupPredicted - 1);
        
        int backflags = Globals::localPlayer->flags();
        Vector OldLocalPlaerOrigin = Globals::localPlayer->origin();
        Vector OldLocalPlayerVelocity = Globals::localPlayer->velocity();
        
        int once = 0;
        if (!HitJump && !HitMiniJump && !HitLongJump && !HitCHop) {
            for (int i = 0; i < 48; i++) {
                if (Globals::localPlayer->flags() & 1)
                    once += 1;

                if (once == 1)
                    ChatPositions = Globals::localPlayer->origin();

                if (v == 0) {
                    if (Globals::localPlayer->flags() & 1)
                        cmd->buttons |= IN_JUMP;
                }
                if (v == 1) {
                    if (Globals::localPlayer->flags() & 1) {
                        cmd->buttons |= IN_JUMP;
                        cmd->buttons |= IN_DUCK;
                    }
                }
                if (v == 2) {
                    if (Globals::localPlayer->flags() & 1) {
                        cmd->buttons |= IN_JUMP;
                        cmd->buttons |= IN_DUCK;
                    }
                }
                if (!(Globals::localPlayer->flags() & 1) && v != 3 && v != 4) {
                    cmd->buttons &= ~IN_DUCK;
                    cmd->buttons &= ~IN_JUMP;
                }
                if (v == 3) {
                    if (Globals::localPlayer->flags() & 1)
                        cmd->buttons |= IN_JUMP;
                    cmd->buttons |= IN_DUCK;
                }
                if (v == 4) {
                    if (i == hitgroudtick) {
                        cmd->buttons |= IN_DUCK;
                        cmd->buttons &= ~IN_JUMP;
                        once += 1;
                    } else {
                        cmd->buttons &= ~IN_DUCK;
                        cmd->buttons |= IN_JUMP;
                    }
                }

                backflags = Globals::localPlayer->flags();
                OldLocalPlaerOrigin = Globals::localPlayer->origin();
                OldLocalPlayerVelocity = Globals::localPlayer->velocity();

                Features::Prediction::start(cmd);
                Features::Prediction::end();
                BackupPredicted = Interfaces::prediction->Split[0].nCommandsPredicted;

                if (v == 2) {
                    if (Globals::localPlayer->flags() & 1 && !(backflags & 1))
                        hitgroudtick = i;
                }

                if (!(Globals::localPlayer->flags() & 1) && v != 3 && v != 4) {
                    cmd->buttons &= ~IN_DUCK;
                    cmd->buttons &= ~IN_JUMP;
                }

                if (backflags & 1 && !(Globals::localPlayer->flags() & 1) && v == 2) {
                    Vector newOrigin = Globals::localPlayer->origin();
                    newOrigin.z += 8.9999704f;
                }

                Vector PredictedLocalPlayerOrigin = Globals::localPlayer->origin();
                Vector PredictedLocalPlayerVelocity = Globals::localPlayer->velocity();
                iCalcilate = 0.f;

                if (OldLocalPlayerVelocity.z > 0.f && PredictedLocalPlayerVelocity.z < 0.f && !iCalcilate && once == 1) {
                    Calculate = OldLocalPlaerOrigin;
                    iCalcilate = OldLocalPlaerOrigin.z - PredictedLocalPlayerOrigin.z;
                    once += 1;
                }

                if (iCalcilate) {
                    float z = 0;
                    while (Calculate.z > Surf.z - 20.f) {
                        if (check(Calculate.z, Surf.z)) {
                            if (v == 0)
                                HitJump = true;
                            if (v == 1)
                                HitMiniJump = true;
                            if (v == 2)
                                HitLongJump = true;
                            if (v == 3)
                                HitCHop = true;
                            if (v == 4)
                                HitJumpBug = true;
                            ChatStand = true;
                            ticks = cmd->tick_count + i + 2;
                        }
                        if (check(Calculate.z + 9.f, Surf.z)) {
                            if (v == 0)
                                HitJump = true;
                            if (v == 1)
                                HitMiniJump = true;
                            if (v == 2)
                                HitLongJump = true;
                            if (v == 4)
                                HitJumpBug = true;
                            ChatDuck = true;
                            ticks = cmd->tick_count + i + 2;
                        }
                        if (v == 3 && check(Calculate.z - 9.f, Surf.z)) {
                            ChatStand = true;
                            HitCHop = true;
                            ticks = cmd->tick_count + i + 2;
                        }

                        float o = z * (z + 1) / 2;
                        float p = iCalcilate + (0.1953125 * z);
                        Calculate.z = Calculate.z - p;
                        z += 1;
                    }
                }
            }
        }
    }

    cmd->buttons = BackupButtons;
    cmd->forwardmove = ForwaMove;
    cmd->sidemove = SideMove;
    Features::Prediction::restoreEntityToPredictedFrame(BackupPredicted - 1);

    if (flZVelBackup > -19.f) {
        ticks = 0;
        HITGODA = false;
    }

    if (cmd->tick_count < ticks) {
        HITGODA = true;
        cmd->forwardmove = 0.f;
        cmd->sidemove = 0.f;
        cmd->buttons = 0;
        cmd->mousedx = 0;
        cmd->mousedy = 0;
    } else {
        HITGODA = false;
    }

    if (flZVelBackup > 0.f) {
        if (HitJumpBug && !ChatPositions.IsZero()) {
            // Could add notification here
        }
        HitCHop = false;
        HitJumpBug = false;
    }

    cmd->buttons = BackupButtons;
    if (HitCHop) {
        cmd->buttons |= IN_DUCK;
        if (Features::Movement::flagsBackup & 1)
            cmd->buttons |= IN_JUMP;
    }
    if (HitJump && Features::Movement::flagsBackup & 1) {
        cmd->buttons |= IN_JUMP;
        HitJump = false;
    }
    if (HitMiniJump && Features::Movement::flagsBackup & 1) {
        cmd->buttons |= IN_JUMP;
        cmd->buttons |= IN_DUCK;
        HitMiniJump = false;
    }
    if (HitLongJump && Features::Movement::flagsBackup & 1) {
        cmd->buttons |= IN_JUMP;
        cmd->buttons |= IN_DUCK;
        HitLongJump = false;
    }
    if (HitJumpBug && Features::Movement::flagsBackup & 1) {
        cmd->buttons |= IN_DUCK;
        cmd->buttons &= ~IN_JUMP;
        HitJumpBug = false;
    }
}

bool checkEdgebug() {
    static ConVar *sv_gravity = Interfaces::convar->FindVar("sv_gravity");
    float edgebugZVel =
       (sv_gravity->GetFloat() * 0.5f * Interfaces::globals->interval_per_tick);

    return Features::Movement::velBackup.z < -edgebugZVel &&
           round(Globals::localPlayer->velocity().z) == -round(edgebugZVel) &&
           Globals::localPlayer->moveType() != MOVETYPE_LADDER;
}

void Features::Movement::prePredCreateMove(CUserCmd *cmd) {
    if (!Globals::localPlayer)
        return;

    flagsBackup = Globals::localPlayer->flags();
    velBackup = Globals::localPlayer->velocity();

    static ConVar *sv_gravity = Interfaces::convar->FindVar("sv_gravity");
    flZVelBackup = Globals::localPlayer->velocity().z;
    flBugSpeed = (sv_gravity->GetFloat() * 0.5f * Interfaces::globals->interval_per_tick);

    bhop(cmd);

    if (shouldEdgebug && shouldDuckNext)
        cmd->buttons |= IN_DUCK;
}

void Features::Movement::postPredCreateMove(CUserCmd *cmd) {
    if (!Globals::localPlayer || Globals::localPlayer->moveType() == MOVETYPE_LADDER ||
        Globals::localPlayer->moveType() == MOVETYPE_NOCLIP)
        return;

    edgeJump(cmd);
    longJump(cmd);
    miniJump(cmd);
    pixelSurf(cmd);
    autoAlign(cmd);
    fireMan(cmd);
    autoBounce(cmd);
    checkSurf(cmd);
    jumpBug(cmd);
}

void Features::Movement::edgeBugPredictor(CUserCmd *cmd) {
    if (!CONFIGBOOL("Misc>Misc>Movement>EdgeBug") ||
        !Menu::CustomWidgets::isKeyDown(CONFIGINT("Misc>Misc>Movement>EdgeBug Key")) ||
        !Globals::localPlayer->health())
        return;

    struct MovementVars {
        QAngle viewangles;
        QAngle view_delta;
        float forwardmove;
        float sidemove;
        int buttons;
    };
    static MovementVars backup_move;
    MovementVars original_move;
    original_move.viewangles = cmd->viewangles;
    original_move.view_delta = (cmd->viewangles - Globals::oldViewangles);
    original_move.forwardmove = cmd->forwardmove;
    original_move.sidemove = cmd->sidemove;
    original_move.buttons = cmd->buttons;
    if (!shouldEdgebug)
        backup_move = original_move;

    int nCmdsPred = Interfaces::prediction->Split->nCommandsPredicted;

    int predictAmount = 128; // TODO: make amount configurable
    for (int t = 0; t < 4; t++) {
        Features::Prediction::restoreEntityToPredictedFrame(nCmdsPred - 1);

        bool doStrafe = (t % 2 == 0);
        bool doDuck = t > 1;

        cmd->viewangles = backup_move.viewangles;

        for (int i = 0; i < predictAmount; i++) {
            if (doStrafe) {
                cmd->viewangles += backup_move.view_delta;
                cmd->forwardmove = backup_move.forwardmove;
                cmd->sidemove = backup_move.sidemove;
            } else {
                cmd->forwardmove = 0.f;
                cmd->sidemove = 0.f;
            }
            if (doDuck)
                cmd->buttons |= IN_DUCK;
            else
                cmd->buttons &= ~IN_DUCK;

            Features::Prediction::start(cmd);
            shouldEdgebug = checkEdgebug();
            velBackup = Globals::localPlayer->velocity();
            edgebugPos = Globals::localPlayer->origin();
            Features::Prediction::end();
            if (Globals::localPlayer->flags() & FL_ONGROUND || Globals::localPlayer->moveType() == MOVETYPE_LADDER) {
                break;
            }
            if (shouldEdgebug) {
                shouldDuckNext = doDuck;
                if (doStrafe) {
                    cmd->viewangles = backup_move.viewangles + backup_move.view_delta;
                    backup_move.viewangles = cmd->viewangles;
                }
                if (i == 1)
                    Interfaces::engine->ExecuteClientCmd(
                       "play buttons/blip1.wav"); // TODO: play sound via a better method
                return;
            }
        }
    }

    cmd->viewangles = original_move.viewangles;
    cmd->forwardmove = original_move.forwardmove;
    cmd->sidemove = original_move.sidemove;
    cmd->buttons = original_move.buttons;
}

void Features::Movement::draw() {
    static float longjump_alpha = 0.f;
    static float minijump_alpha = 0.f;
    static float jumpbug_alpha = 0.f;
    static float pixelsurf_alpha = 0.f;
    static float lastTime = 0.f;
    
    float currentTime = Interfaces::globals->realtime;
    float elapsedTime = currentTime - lastTime;
    lastTime = currentTime;
    
    float alphaChangePerSecond = 8.f;
    
    float h = Globals::screenSizeY / 2 + 30;
    
    // Longjump indicator
    if (CONFIGBOOL("Misc>Misc>Movement>LongJump") &&
        CONFIGBOOL("Misc>Misc>Movement>Edge Jump") &&
        Menu::CustomWidgets::isKeyDown(CONFIGINT("Misc>Misc>Movement>Edge Jump Key"))) {
        longjump_alpha += static_cast<float>(alphaChangePerSecond * elapsedTime);
    } else {
        longjump_alpha -= static_cast<float>(alphaChangePerSecond * elapsedTime);
    }
    
    if (longjump_alpha > 1.0f) longjump_alpha = 1.0f;
    if (longjump_alpha < 0.0f) longjump_alpha = 0.0f;
    
    if (longjump_alpha > 0.f && CONFIGBOOL("Misc>Misc>Movement>Show Indicators")) {
        h += ImGui::CalcTextSize("lj").y * longjump_alpha;
        
        int alpha = static_cast<int>(longjump_alpha * 255);
        ImColor textWhite = should_longjump ? ImColor(255, 255, 255, alpha) : ImColor(200, 200, 200, alpha);
        ImColor textBlack = should_longjump ? ImColor(0, 0, 0, alpha) : ImColor(0, 0, 0, alpha);
        
        Globals::drawList->AddText(
           ImVec2((Globals::screenSizeX / 2) - (ImGui::CalcTextSize("lj").x / 2) + 1, h + 1),
           textBlack, "lj");
        Globals::drawList->AddText(
           ImVec2((Globals::screenSizeX / 2) - (ImGui::CalcTextSize("lj").x / 2), h),
           textWhite, "lj");
    }
    
    // Minijump indicator
    if (CONFIGBOOL("Misc>Misc>Movement>MiniJump") &&
        Menu::CustomWidgets::isKeyDown(CONFIGINT("Misc>Misc>Movement>MiniJump Key"))) {
        minijump_alpha += static_cast<float>(alphaChangePerSecond * elapsedTime);
    } else {
        minijump_alpha -= static_cast<float>(alphaChangePerSecond * elapsedTime);
    }
    
    if (minijump_alpha > 1.0f) minijump_alpha = 1.0f;
    if (minijump_alpha < 0.0f) minijump_alpha = 0.0f;
    
    if (minijump_alpha > 0.f && CONFIGBOOL("Misc>Misc>Movement>Show Indicators")) {
        h += ImGui::CalcTextSize("mj").y * minijump_alpha;
        
        int alpha = static_cast<int>(minijump_alpha * 255);
        ImColor textWhite = should_minijump ? ImColor(255, 255, 255, alpha) : ImColor(200, 200, 200, alpha);
        ImColor textBlack = should_minijump ? ImColor(0, 0, 0, alpha) : ImColor(0, 0, 0, alpha);
        
        Globals::drawList->AddText(
           ImVec2((Globals::screenSizeX / 2) - (ImGui::CalcTextSize("mj").x / 2) + 1, h + 1),
           textBlack, "mj");
        Globals::drawList->AddText(
           ImVec2((Globals::screenSizeX / 2) - (ImGui::CalcTextSize("mj").x / 2), h),
           textWhite, "mj");
    }
    
    // JumpBug indicator
    if (CONFIGBOOL("Misc>Misc>Movement>JumpBug") &&
        Menu::CustomWidgets::isKeyDown(CONFIGINT("Misc>Misc>Movement>JumpBug Key"))) {
        jumpbug_alpha += static_cast<float>(alphaChangePerSecond * elapsedTime);
    } else {
        jumpbug_alpha -= static_cast<float>(alphaChangePerSecond * elapsedTime);
    }
    
    if (jumpbug_alpha > 1.0f) jumpbug_alpha = 1.0f;
    if (jumpbug_alpha < 0.0f) jumpbug_alpha = 0.0f;
    
    if (jumpbug_alpha > 0.f && CONFIGBOOL("Misc>Misc>Movement>Show Indicators")) {
        h += ImGui::CalcTextSize("jb").y * jumpbug_alpha;
        
        int alpha = static_cast<int>(jumpbug_alpha * 255);
        ImColor textWhite = ImColor(255, 255, 255, alpha);
        ImColor textBlack = ImColor(0, 0, 0, alpha);
        
        Globals::drawList->AddText(
           ImVec2((Globals::screenSizeX / 2) - (ImGui::CalcTextSize("jb").x / 2) + 1, h + 1),
           textBlack, "jb");
        Globals::drawList->AddText(
           ImVec2((Globals::screenSizeX / 2) - (ImGui::CalcTextSize("jb").x / 2), h),
           textWhite, "jb");
    }
    
    // PixelSurf indicator
    if (CONFIGBOOL("Misc>Misc>Movement>PixelSurf")) {
        pixelsurf_alpha += static_cast<float>(alphaChangePerSecond * elapsedTime);
    } else {
        pixelsurf_alpha -= static_cast<float>(alphaChangePerSecond * elapsedTime);
    }
    
    if (pixelsurf_alpha > 1.0f) pixelsurf_alpha = 1.0f;
    if (pixelsurf_alpha < 0.0f) pixelsurf_alpha = 0.0f;
    
    if (pixelsurf_alpha > 0.f && CONFIGBOOL("Misc>Misc>Movement>Show Indicators")) {
        h += ImGui::CalcTextSize("ps").y * pixelsurf_alpha;
        
        int alpha = static_cast<int>(pixelsurf_alpha * 255);
        ImColor textWhite = should_pixelsurf ? ImColor(255, 255, 255, alpha) : ImColor(200, 200, 200, alpha);
        ImColor textBlack = should_pixelsurf ? ImColor(0, 0, 0, alpha) : ImColor(0, 0, 0, alpha);
        
        Globals::drawList->AddText(
           ImVec2((Globals::screenSizeX / 2) - (ImGui::CalcTextSize("ps").x / 2) + 1, h + 1),
           textBlack, "ps");
        Globals::drawList->AddText(
           ImVec2((Globals::screenSizeX / 2) - (ImGui::CalcTextSize("ps").x / 2), h),
           textWhite, "ps");
    }
    
    // EdgeBug indicator
    if (Features::Movement::shouldEdgebug && CONFIGBOOL("Misc>Misc>Movement>Show Indicators")) {
        Globals::drawList->AddText(
           ImVec2((Globals::screenSizeX / 2) - (ImGui::CalcTextSize("EdgeBug").x / 2) + 1,
                  (Globals::screenSizeY / 2) + 31),
           ImColor(0, 0, 0, 255), "EdgeBug");
        Globals::drawList->AddText(
           ImVec2((Globals::screenSizeX / 2) - (ImGui::CalcTextSize("EdgeBug").x / 2),
                  (Globals::screenSizeY / 2) + 30),
           ImColor(255, 255, 255, 255), "EdgeBug");

        Vector edgebugPos2D;
        if (worldToScreen(edgebugPos, edgebugPos2D)) {
            Globals::drawList->AddText(
               ImVec2(edgebugPos2D.x - (ImGui::CalcTextSize("gaming").x / 2) + 1,
                      edgebugPos2D.y + 1),
               ImColor(0, 0, 0, 255), "gaming");
            Globals::drawList->AddText(
               ImVec2(edgebugPos2D.x - (ImGui::CalcTextSize("gaming").x / 2),
                      edgebugPos2D.y),
               ImColor(255, 255, 255, 255), "gaming");
        }
    }
}