#include "features.hpp"
#include "../../includes.hpp"

namespace {
    inline const char* knives[] = {
        "",
        "bayonet",
        "flip",
        "gut",
        "karambit",
        "m9 bayonet",
        "huntsman",
        "falchion",
        "bowie",
        "butterfly",
        "push",
        "ursus",
        "navaja",
        "stiletto",
        "talon",
        "classic",
        "ghost",
        "gold"
    };

    inline std::map<std::string, Features::SkinChanger::Item> nameToItemMap = {
        {"bayonet",         {ItemIndex::WEAPON_KNIFE_BAYONET,        "#SFUI_WPNHUD_KnifeBayonet",            "weapon_knife_bayonet",         "models/weapons/v_knife_bayonet.mdl",           "bayonet"}},
        {"flip",            {ItemIndex::WEAPON_KNIFE_FLIP,           "#SFUI_WPNHUD_KnifeFlip",               "weapon_knife_flip",            "models/weapons/v_knife_flip.mdl",              "knife_flip"}},
        {"gut",             {ItemIndex::WEAPON_KNIFE_GUT,            "#SFUI_WPNHUD_KnifeGut",                "weapon_knife_gut",             "models/weapons/v_knife_gut.mdl",               "knife_gut"}},
        {"karambit",        {ItemIndex::WEAPON_KNIFE_KARAMBIT,       "#SFUI_WPNHUD_KnifeKaram",              "weapon_knife_karambit",        "models/weapons/v_knife_karam.mdl",             "knife_karambit"}},
        {"m9 bayonet",      {ItemIndex::WEAPON_KNIFE_M9_BAYONET,     "#SFUI_WPNHUD_KnifeM9",                 "weapon_knife_m9_bayonet",      "models/weapons/v_knife_m9_bay.mdl",            "knife_m9_bayonet"}},
        {"huntsman",        {ItemIndex::WEAPON_KNIFE_TACTICAL,       "#SFUI_WPNHUD_KnifeTactical",           "weapon_knife_tactical",        "models/weapons/v_knife_tactical.mdl",          "knife_tactical"}},
        {"falchion",        {ItemIndex::WEAPON_KNIFE_FALCHION,       "#SFUI_WPNHUD_knife_falchion_advanced", "weapon_knife_falchion",        "models/weapons/v_knife_falchion_advanced.mdl", "knife_falchion"}},
        {"bowie",           {ItemIndex::WEAPON_KNIFE_SURVIVAL_BOWIE, "#SFUI_WPNHUD_knife_survival_bowie",    "weapon_knife_survival_bowie",  "models/weapons/v_knife_survival_bowie.mdl",    "knife_survival_bowie"}},
        {"butterfly",       {ItemIndex::WEAPON_KNIFE_BUTTERFLY,      "#SFUI_WPNHUD_Knife_Butterfly",         "weapon_knife_butterfly",       "models/weapons/v_knife_butterfly.mdl",         "knife_butterfly"}},
        {"push",            {ItemIndex::WEAPON_KNIFE_PUSH,           "#SFUI_WPNHUD_knife_push",              "weapon_knife_push",            "models/weapons/v_knife_push.mdl",              "knife_push"}},
        {"ursus",           {ItemIndex::WEAPON_KNIFE_URSUS,          "#SFUI_WPNHUD_knife_ursus",             "weapon_knife_ursus",           "models/weapons/v_knife_ursus.mdl",             "knife_ursus"}},
        {"navaja",          {ItemIndex::WEAPON_KNIFE_GYPSY_JACKKNIFE,"#SFUI_WPNHUD_knife_gypsy_jackknife",   "weapon_knife_gypsy_jackknife", "models/weapons/v_knife_gypsy_jackknife.mdl",   "knife_gypsy_jackknife"}},
        {"stiletto",        {ItemIndex::WEAPON_KNIFE_STILETTO,       "#SFUI_WPNHUD_knife_stiletto",          "weapon_knife_stiletto",        "models/weapons/v_knife_stiletto.mdl",          "knife_stiletto"}},
        {"talon",           {ItemIndex::WEAPON_KNIFE_WIDOWMAKER,     "#SFUI_WPNHUD_knife_widowmaker",        "weapon_knife_widowmaker",      "models/weapons/v_knife_widowmaker.mdl",        "knife_widowmaker"}},
        {"classic",         {ItemIndex::WEAPON_KNIFE_CSS,            "#SFUI_WPNHUD_KnifeCSS",                "weapon_knife_css",             "models/weapons/v_knife_css.mdl",               "knife_css"}},
        {"ghost",           {ItemIndex::WEAPON_KNIFE_GHOST,          "#SFUI_WPNHUD_knife_ghost",             "weapon_knife_ghost",           "models/weapons/v_knife_ghost.mdl",             "knife_ghost"}},
        {"gold",            {ItemIndex::WEAPON_KNIFEGG,              "#SFUI_WPNHUD_Knife_GG",                "weapon_knifegg",               "models/weapons/v_knife_gg.mdl",                "knifegg"}}
    };
}

void Features::SkinChanger::applyModel(Weapon* weapon, Weapon* viewModel, Item item) {
    if (!weapon || !Interfaces::modelInfo)
        return;

    int modelIndex = Interfaces::modelInfo->GetModelIndex(item.modelName);
    if (modelIndex == -1)
        return;

    // Use direct netvar access like ECL instead of virtual functions
    // This is safer during weapon switch transitions
    weapon->itemIndex() = item.index;
    weapon->modelIndex() = modelIndex;
    
    // ECL also modifies viewModel - try with direct netvar access
    if (viewModel) {
        viewModel->modelIndex() = modelIndex;
    }
}

void Features::SkinChanger::frameStageNotify(FrameStage frame) {
    if (!Globals::localPlayer || !Interfaces::engine || !Interfaces::entityList || !Interfaces::modelInfo)
        return;

    if (frame != FRAME_NET_UPDATE_POSTDATAUPDATE_START)
        return;

    if (!Interfaces::engine->IsInGame())
        return;

    if (Globals::localPlayer->health() <= 0)
        return;

    // Get current weapon
    void* weaponHandle = Globals::localPlayer->activeWeapon();
    if (weaponHandle == (void*)0xFFFFFFFF)
        return;

    Weapon* curWeapon = (Weapon*) Interfaces::entityList->GetClientEntity((uintptr_t)weaponHandle & 0xfff);
    if (!curWeapon)
        return;

    if (curWeapon->itemIndex() == ItemIndex::INVALID)
        return;

    // ECL doesn't check clientClass, just itemIndex
    if (curWeapon->itemIndex() != ItemIndex::WEAPON_KNIFE && 
        curWeapon->itemIndex() != ItemIndex::WEAPON_KNIFE_BAYONET &&
        curWeapon->itemIndex() != ItemIndex::WEAPON_KNIFE_FLIP &&
        curWeapon->itemIndex() != ItemIndex::WEAPON_KNIFE_GUT &&
        curWeapon->itemIndex() != ItemIndex::WEAPON_KNIFE_KARAMBIT &&
        curWeapon->itemIndex() != ItemIndex::WEAPON_KNIFE_M9_BAYONET &&
        curWeapon->itemIndex() != ItemIndex::WEAPON_KNIFE_TACTICAL &&
        curWeapon->itemIndex() != ItemIndex::WEAPON_KNIFE_FALCHION &&
        curWeapon->itemIndex() != ItemIndex::WEAPON_KNIFE_SURVIVAL_BOWIE &&
        curWeapon->itemIndex() != ItemIndex::WEAPON_KNIFE_BUTTERFLY &&
        curWeapon->itemIndex() != ItemIndex::WEAPON_KNIFE_PUSH &&
        curWeapon->itemIndex() != ItemIndex::WEAPON_KNIFE_URSUS &&
        curWeapon->itemIndex() != ItemIndex::WEAPON_KNIFE_GYPSY_JACKKNIFE &&
        curWeapon->itemIndex() != ItemIndex::WEAPON_KNIFE_STILETTO &&
        curWeapon->itemIndex() != ItemIndex::WEAPON_KNIFE_WIDOWMAKER &&
        curWeapon->itemIndex() != ItemIndex::WEAPON_KNIFE_CSS &&
        curWeapon->itemIndex() != ItemIndex::WEAPON_KNIFE_GHOST &&
        curWeapon->itemIndex() != ItemIndex::WEAPON_KNIFEGG)
        return;

    // Get view model - ECL requires this to be valid
    void* viewModelHandle = Globals::localPlayer->viewModelHandle();
    if (viewModelHandle == (void*)0xFFFFFFFF)
        return;

    Weapon* viewModel = (Weapon*) Interfaces::entityList->GetClientEntity((uintptr_t)viewModelHandle & 0xfff);
    if (!viewModel)
        return;

    int knifeModel = CONFIGINT("Misc>Skins>Knife Model");
    if (knifeModel <= 0 || knifeModel >= 18)
        return;

    const char* knifeName = knives[knifeModel];
    if (nameToItemMap.find(knifeName) == nameToItemMap.end())
        return;

    applyModel(curWeapon, viewModel, nameToItemMap[knifeName]);
}