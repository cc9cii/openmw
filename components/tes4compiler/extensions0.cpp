#include "extensions0.hpp"

#include "opcodes.hpp"
#include "../compiler/extensions.hpp"

namespace Tes4Compiler
{
    void registerExtensions (Compiler::Extensions& extensions, bool consoleOnly)
    {
        Tes4Event::registerExtensions (extensions);
        Tes4Actor::registerExtensions (extensions);
        Tes4AI::registerExtensions (extensions);
        Tes4Animation::registerExtensions (extensions);
        Tes4Combat::registerExtensions (extensions);
        Tes4Crime::registerExtensions (extensions);
        Tes4Dialogue::registerExtensions (extensions);
        Tes4Faction::registerExtensions (extensions);
        Tes4Inventory::registerExtensions (extensions);
        Tes4Magic::registerExtensions (extensions);
        Tes4Movement::registerExtensions (extensions);
        Tes4Player::registerExtensions (extensions);
        Tes4Quest::registerExtensions (extensions);
        Tes4Statistics::registerExtensions (extensions);
        Tes4Weather::registerExtensions (extensions);
        Tes4Misc::registerExtensions (extensions);
    }

    namespace Tes4Event
    {
        void registerExtensions (Compiler::Extensions& extensions)
        {
            // NOTE: some of these don't need to be functions
            extensions.registerFunction ("gamemode",         'l', "",    opcodeGameMode);
            extensions.registerFunction ("menumode",         'l', "/l",  opcodeMenuMode);
            extensions.registerFunction ("onactivate",       'l', "/c",  opcodeOnActivate);
            extensions.registerFunction ("onactorequip",     'l', "",    opcodeOnActorEquip);
            extensions.registerFunction ("onactorunequip",   'l', "",    opcodeOnActorUnequip);
            extensions.registerFunction ("onadd",            'l', "/c",  opcodeOnAdd);
            extensions.registerFunction ("onalarm",          'l', "l/c", opcodeOnAlarm);
            extensions.registerFunction ("onalarmvictim",    'l', "l/c", opcodeOnAlarmVictim);
            extensions.registerFunction ("ondeath",          'l', "/c",  opcodeOnDeath);
            extensions.registerFunction ("ondrop",           'l', "/c",  opcodeOnDrop);
            extensions.registerFunction ("onequip",          'l', "/c",  opcodeOnEquip);
            extensions.registerFunction ("onhit",            'l', "/c",  opcodeOnHit);
            extensions.registerFunction ("onhitwith",        'l', "/c",  opcodeOnHitWith);
            extensions.registerFunction ("onknockout",       'l', "",    opcodeOnKnockout);
            extensions.registerFunction ("onload",           'l', "",    opcodeOnLoad);
            extensions.registerFunction ("onmagiceffecthit", 'l', "/c",  opcodeOnMagicEffectHit);
            extensions.registerFunction ("onmurder",         'l', "/c",  opcodeOnMurder);
            extensions.registerFunction ("onpackagechange",  'l', "c",   opcodeOnPackageChange);
            extensions.registerFunction ("onpackagedone",    'l', "c",   opcodeOnPackageDone);
            extensions.registerFunction ("onpackageend",     'l', "c",   opcodeOnPackageEnd);
            extensions.registerFunction ("onpackagestart",   'l', "c",   opcodeOnPackageStart);
            extensions.registerFunction ("onreset",          'l', "",    opcodeOnReset);
            extensions.registerFunction ("onsell",           'l', "/c",  opcodeOnSell);
            extensions.registerFunction ("onstartcombat",    'l', "/c",  opcodeOnStartCombat);
            extensions.registerFunction ("ontrigger",        'l', "/c",  opcodeOnTrigger);
            extensions.registerFunction ("ontriggeractor",   'l', "/c",  opcodeOnTriggerActor);
            extensions.registerFunction ("ontriggermob",     'l', "/c",  opcodeOnTriggerMob);
            extensions.registerFunction ("onunequip",        'l', "/c",  opcodeOnUnequip);
            extensions.registerFunction ("scripteffectfinish", 'l', "",  opcodeScriptEffectFinish);
            extensions.registerFunction ("scripteffectstart",  'l', "",  opcodeScriptEffectStart);
            extensions.registerFunction ("scripteffectupdate", 'l', "",  opcodeScriptEffectUpdate);
        }
    }

    namespace Tes4Actor
    {
        void registerExtensions (Compiler::Extensions& extensions)
        {
            extensions.registerInstruction
                ("createfullactorcopy",   "",       opcodeCreateFullActorCopy, opcodeCreateFullActorCopyExplicit);
            extensions.registerInstruction
                ("deletefullactorcopy",   "",       opcodeDeleteFullActorCopy, opcodeDeleteFullActorCopyExplicit);
            extensions.registerInstruction
                ("duplicatenpcstats",     "/c",     opcodeDuplicateNPCStats, opcodeDuplicateNPCStatsExplicit);
            extensions.registerFunction
                ("isactor",               'l', "",  opcodeIsActor, opcodeIsActorExplicit);
            extensions.registerFunction
                ("isactorsaioff",         'l', "",  opcodeIsActorsAIOff, opcodeIsActorsAIOffExplicit);
            extensions.registerInstruction
                ("killallactors",         "/c",     opcodeKillAllActors, -1);
            extensions.registerInstruction
                ("killall",               "/c",     opcodeKillAllActors, -1);
            extensions.registerInstruction
                ("setessential",          "c/l",    opcodeSetEssential, -1);
            extensions.registerInstruction
                ("toggleactorsai",        "",       opcodeToggleActorsAI, opcodeToggleActorsAIExplicit);
            // state
            extensions.registerFunction
                ("getcloathingvalue",     'l', "",  opcodeGetClothingValue, opcodeGetClothingValueExplicit);
            extensions.registerFunction
                ("getdead",               'l', "",  opcodeGetDead, opcodeGetDeadExplicit);
            extensions.registerFunction
                ("getdeadcount",          'l', "c", opcodeGetDeadCount, -1);
            extensions.registerFunction
                ("getdetected",           'l', "c", opcodeGetDetected, opcodeGetDetectedExplicit);
            extensions.registerFunction
                ("getforcerun",           'l', "",  opcodeGetForceRun, opcodeGetForceRunExplicit);
            extensions.registerFunction
                ("getforcesneak",         'l', "",  opcodeGetForceSneak, opcodeGetForceSneakExplicit);
            extensions.registerFunction
                ("getincell",             'l', "c", opcodeGetInCell, opcodeGetInCellExplicit);
            extensions.registerFunction
                ("getincellparam",        'l', "cc", opcodeGetInCellParam, -1);
            extensions.registerFunction
                ("getinsamecell",         'l', "c", opcodeGetInSameCell, opcodeGetInSameCellExplicit);
            extensions.registerFunction
                ("getinworldspace",       'l', "c", opcodeGetInWorldspace, opcodeGetInWorldspaceExplicit);
            extensions.registerFunction
                ("getisalerted",          'l', "",  opcodeGetIsAlerted, opcodeGetIsAlertedExplicit);
            extensions.registerFunction
                ("getisclass",            'l', "c", opcodeGetIsClass, opcodeGetIsClassExplicit);
            extensions.registerFunction
                ("getiscreature",         'l', "",  opcodeGetIsCreature, opcodeGetIsCreatureExplicit);
            extensions.registerFunction
                ("getisghost",            'l', "",  opcodeGetIsGhost, opcodeGetIsGhostExplicit);
            extensions.registerFunction
                ("getisid",               'l', "c", opcodeGetIsID, opcodeGetIsIDExplicit);
            extensions.registerFunction
                ("getisplayablerace",     'l', "",  opcodeGetIsPlayableRace, opcodeGetIsPlayableRaceExplicit);
            extensions.registerFunction
                ("getisrace",             'l', "c", opcodeGetIsRace, opcodeGetIsRaceExplicit);
            extensions.registerFunction
                ("getissex",              'l', "c", opcodeGetIsSex, opcodeGetIsSexExplicit);
            extensions.registerFunction
                ("getisuseditem",         'l', "c", opcodeGetIsUsedItem, -1); // no explicit?
            extensions.registerFunction
                ("getisuseditemtype",     'l', "c", opcodeGetIsUsedItemType, -1); // no explicit?
            extensions.registerFunction
                ("getknockedstate",       'l', "",  opcodeGetKnockedState, opcodeGetKnockedStateExplicit);
            extensions.registerFunction
                ("getnorumors",           'l', "",  opcodeGetNoRumors, opcodeNoRumorsExplicit);
            extensions.registerFunction
                ("getoffersservicesnow",  'l', "",  opcodeGetOffersServicesNow, opcodeOffersServicesNowExplicit);
            extensions.registerFunction
                ("getsitting",            'l', "",  opcodeGetSitting, opcodeGetSittingExplicit);
            extensions.registerFunction
                ("getsleeping",           'l', "",  opcodeGetSleeping, opcodeGetSleepingExplicit);
            extensions.registerFunction
                ("gettalkedtopc",         'l', "",  opcodeGetTalkedToPC, opcodeGetTalkedToPCExplicit);
            extensions.registerFunction
                ("gettalkedtopcparam",    'l', "",  opcodeGetTalkedToPCParam, -1);
            extensions.registerFunction
                ("gettimedead",           'f', "",  opcodeGetTimeDead, opcodeGetTimeDeadExplicit);
            extensions.registerFunction
                ("getvampire",            'l', "",  opcodeGetVampire, opcodeGetVampireExplicit);
            extensions.registerFunction
                ("getweaponanimtype",     'l', "",  opcodeGetWeaponAnimType, opcodeGetWeaponAnimTypeExplicit);
            extensions.registerFunction
                ("getweaponskilltype",    'l', "",  opcodeGetWeaponSkillType, opcodeGetWeaponSkillTypeExplicit);
            extensions.registerFunction
                ("isactoravictim",        'l', "",  opcodeIsActorAVictim, opcodeIsActorAVictimExplicit);
            extensions.registerFunction
                ("isactorevil",           'l', "",  opcodeIsActorEvil, opcodeIsActorEvilExplicit);
            extensions.registerFunction
                ("isactorusingatorch",    'l', "",  opcodeIsActorUsingATorch, opcodeIsActorUsingATorchExplicit);
            extensions.registerFunction
                ("iscurrentfurnitureobj", 'l', "c", opcodeIsCurrentFurnitureObj, opcodeIsCurrentFurnitureObjExplicit);
            extensions.registerFunction
                ("iscurrentfurnitureref", 'l', "c", opcodeIsCurrentFurnitureRef, opcodeIsCurrentFurnitureRefExplicit);
            extensions.registerFunction
                ("isessential",           'l', "",  opcodeIsEssential, opcodeIsEssentialExplicit);
            extensions.registerFunction
                ("isfacingup",            'l', "",  opcodeIsFacingUp, opcodeIsFacingUpExplicit);
            extensions.registerFunction
                ("isguard",               'l', "",  opcodeIsGuard, opcodeIsGuardExplicit);
            extensions.registerFunction
                ("isidleplaying",         'l', "",  opcodeIsIdlePlaying, opcodeIsIdlePlayingExplicit);
            extensions.registerFunction
                ("isincombat",            'l', "",  opcodeIsInCombat, opcodeIsInCombatExplicit);
            extensions.registerFunction
                ("isindangerouswater",    'l', "",  opcodeIsInDangerousWater, opcodeIsInDangerousWaterExplicit);
            extensions.registerFunction
                ("isininterior",          'l', "",  opcodeIsInInterior, opcodeIsInInteriorExplicit);
            extensions.registerFunction
                ("isleftup",              'l', "",  opcodeIsLeftUp, opcodeIsLeftUpExplicit);
            extensions.registerFunction
                ("isridinghorse",         'l', "",  opcodeIsRidingHorse, opcodeIsRidingHorseExplicit);
            extensions.registerFunction
                ("isrunning",             'l', "",  opcodeIsRunning, opcodeIsRunningExplicit);
            extensions.registerFunction
                ("isshieldout",           'l', "",  opcodeIsShieldOut, opcodeIsShieldOutExplicit);
            extensions.registerFunction
                ("issneaking",            'l', "",  opcodeIsSneaking, opcodeIsSneakingExplicit);
            extensions.registerFunction
                ("isswimming",            'l', "",  opcodeIsSwimming, opcodeIsSwimmingExplicit);
            extensions.registerFunction
                ("istalking",             'l', "",  opcodeIsTalking, opcodeIsTalkingExplicit);
            extensions.registerFunction
                ("istorchout",            'l', "",  opcodeIsTorchOut, opcodeIsTorchOutExplicit);
            extensions.registerFunction
                ("iswaiting",             'l', "",  opcodeIsWaiting, opcodeIsWaitingExplicit);
            extensions.registerFunction
                ("isweaponout",           'l', "",  opcodeIsWeaponOut, opcodeIsWeaponOutExplicit);
            extensions.registerFunction
                ("isyielding",            'l', "",  opcodeIsYielding, opcodeIsYieldingExplicit);
            extensions.registerInstruction
                ("pickidle",               "",      opcodePickIdle, opcodePickIdleExplicit);
            extensions.registerInstruction
                ("pushactoraway",         "cl",     opcodePushActorAway, opcodePushActorAwayExplicit);
            extensions.registerFunction
                ("samefaction",           'l', "c", opcodeSameFaction, opcodeSameFactionExplicit);
            extensions.registerFunction
                ("samefactionaspc",       'l', "",  opcodeSameFactionAsPC, opcodeSameFactionAsPCExplicit);
            extensions.registerFunction
                ("samerace",              'l', "c", opcodeSameRace, opcodeSameRaceExplicit);
            extensions.registerFunction
                ("sameraceaspc",          'l', "",  opcodeSameRaceAsPC, opcodeSameRaceAsPCExplicit);
            extensions.registerFunction
                ("samesex",               'l', "c", opcodeSameSex, opcodeSameSexExplicit);
            extensions.registerFunction
                ("samesexaspc",           'l', "",  opcodeSameSexAsPC, opcodeSameSexAsPCExplicit);
            extensions.registerInstruction
                ("setclass",              "c",      opcodeSetClass, opcodeSetClassExplicit);
            extensions.registerInstruction
                ("setforcerun",           "l",      opcodeSetForceRun, opcodeSetForceRunExplicit);
            extensions.registerInstruction
                ("setforcesneak",         "l",      opcodeSetForceSneak, opcodeSetForceSneakExplicit);
            extensions.registerInstruction
                ("setghost",              "l",      opcodeSetGhost, opcodeSetGhostExplicit);
            extensions.registerInstruction
                ("setlevel",              "l/lll",  opcodeSetLevel, opcodeSetLevelExplicit);
            // value
            extensions.registerInstruction
                ("forceactorvalue",       "cl",     opcodeForceActorValue, opcodeForceActorValueExplicit);
            extensions.registerInstruction
                ("forceav",               "cl",     opcodeForceActorValue, opcodeForceActorValueExplicit);
            extensions.registerFunction
                ("getactorvalue",         'l', "c", opcodeGetActorValue, opcodeGetActorValueExplicit);
            extensions.registerFunction
                ("getav",                 'l', "c", opcodeGetActorValue, opcodeGetActorValueExplicit);
            extensions.registerFunction
                ("getbaseactorvalue",     'l', "c", opcodeGetBaseActorValue, opcodeGetBaseActorValueExplicit);
            extensions.registerFunction
                ("getbaseav",             'l', "c", opcodeGetBaseActorValue, opcodeGetBaseActorValueExplicit);
            extensions.registerFunction
                ("getdisposition",        'l', "c", opcodeGetDisposition, opcodeGetDispositionExplicit);
            extensions.registerInstruction
                ("modactorvalue",         "cl",     opcodeModActorValue, opcodeModActorValueExplicit);
            extensions.registerInstruction
                ("modav",                 "cl",     opcodeModActorValue, opcodeModActorValueExplicit);
            // the second argument is a short but Generator::convert() doesn't handle it
            extensions.registerInstruction
                ("moddisposition",        "cl",     opcodeModDisposition, opcodeModDispositionExplicit);
            extensions.registerInstruction
                ("resethealth",           "",       opcodeResetHealth, opcodeResetHealthExplicit);
            extensions.registerInstruction
                ("resurrectactor",        "/l",     opcodeResurrectActor, opcodeResurrectActorExplicit);
            extensions.registerInstruction
                ("resurrect",             "/l",     opcodeResurrectActor, opcodeResurrectActorExplicit);
            extensions.registerInstruction
                ("setactorvalue",         "cl",     opcodeSetActorValue, opcodeSetActorValueExplicit);
            extensions.registerInstruction
                ("setav",                 "cl",     opcodeSetActorValue, opcodeSetActorValueExplicit);
        }
    }

    namespace Tes4AI
    {
        void registerExtensions (Compiler::Extensions& extensions)
        {
            extensions.registerInstruction
                ("addscriptpackage",      "c",      opcodeAddScriptPackage, opcodeAddScriptPackageExplicit);
            extensions.registerInstruction
                ("evaluatepackage",       "",       opcodeEvaluatePackage, opcodeEvaluatePackageExplicit);
            extensions.registerInstruction
                ("evp",                   "",       opcodeEvaluatePackage, opcodeEvaluatePackageExplicit);
            extensions.registerInstruction
                ("forceflee",             "/cc",    opcodeForceFlee, opcodeForceFleeExplicit);
            extensions.registerInstruction
                ("flee",                  "/cc",    opcodeForceFlee, opcodeForceFleeExplicit);
            extensions.registerInstruction
                ("forcetakecover",        "cl",     opcodeForceTakeCover, opcodeForceTakeCoverExplicit);
            extensions.registerInstruction
                ("takecover",             "cl",     opcodeForceTakeCover, opcodeForceTakeCoverExplicit);
            extensions.registerFunction
                ("getcurrentaipackage",   'l', "",  opcodeGetCurrentAIPackage, opcodeGetCurrentAIPackageExplicit);
            extensions.registerFunction
                ("getcurrentaiprocedure", 'l', "",  opcodeGetCurrentAIProcedure, opcodeGetCurrentAIProcedureExplicit);
            extensions.registerFunction
                ("getiscurrentpackage",   'l', "c", opcodeGetIsCurrentPackage, opcodeGetIsCurrentPackageExplicit);
            extensions.registerFunction
                ("getrestrained",         'l', "",  opcodeGetRestrained, opcodeGetRestrainedExplicit);
            extensions.registerFunction
                ("getunconscious",        'l', "",  opcodeGetUnconscious, opcodeGetUnconsciousExplicit);
            extensions.registerFunction
                ("iscontinuingpackagepcnear", 'l', "", opcodeIsContinuingPackagePCNear, opcodeIsContinuingPackagePCNearExplicit);
            extensions.registerInstruction
                ("look",                  "c",      opcodeLook, opcodeLookExplicit);
            extensions.registerInstruction
                ("removescriptpackage",    "",      opcodeRemoveScriptPackage, opcodeRemoveScriptPackageExplicit);
            extensions.registerInstruction
                ("setactorsai",           "l",      opcodeSetActorsAI, opcodeSetActorsAIExplicit);
            extensions.registerInstruction
                ("setalert",              "l",      opcodeSetAlert, opcodeSetAlertExplicit);
            extensions.registerInstruction
                ("setallreachable",       "l",      opcodeSetAllReachable, -1);
            extensions.registerInstruction
                ("setallvisible",         "l",      opcodeSetAllVisible, -1);
            extensions.registerInstruction
                ("setnoavoidance",        "l",      opcodeSetNoAvoidance, -1);
            extensions.registerInstruction
                ("setpackduration",       "l",      opcodeSetPackDuration, opcodeSetPackDurationExplicit);
            extensions.registerInstruction
                ("spdur",                 "l",      opcodeSetPackDuration, opcodeSetPackDurationExplicit);
            // has explicit despite https://en.uesp.net/wiki/Tes4Mod:Script_Functions
            // indicating it is not a RefFunc - see MQ09Script
            extensions.registerInstruction
                ("setrestrained",         "l",      opcodeSetRestrained, opcodeSetRestrainedExplicit);
            extensions.registerInstruction
                ("setsceneiscomplex",     "l",      opcodeSetSceneIsComplex, -1);
            // has explicit despite https://en.uesp.net/wiki/Tes4Mod:Script_Functions
            // indicating it is not a RefFunc - see DAVaerminaScript
            extensions.registerInstruction
                ("setunconscious",        "l",      opcodeSetUnconscious, opcodeSetUnconsciousExplicit);
            // https://en.uesp.net/wiki/Tes4Mod:Script_Functions indicates no arguments, but
            // SE07AScript supplies "SEThadonRef"
            extensions.registerInstruction
                ("stoplook",              "/c",     opcodeStopLook, opcodeStopLookExplicit);
            extensions.registerInstruction
                ("stopwaiting",           "c",      opcodeStopWaiting, opcodeStopWaitingExplicit);
            extensions.registerInstruction
                ("vampirefeed",           "c",      opcodeVampireFeed, opcodeVampireFeedExplicit);
            extensions.registerInstruction
                ("wait",                  "c",      opcodeWait, opcodeWaitExplicit);
        }
    }

    namespace Tes4Animation
    {
        void registerExtensions (Compiler::Extensions& extensions)
        {
            extensions.registerFunction
                ("isanimplaying",         'l', "",  opcodeIsAnimPlaying, opcodeIsAnimPlayingExplicit);
            extensions.registerInstruction
                ("loopgroup",             "cll",    opcodeLoopGroup, opcodeLoopGroupExplicit);
            extensions.registerInstruction
                ("playgroup",             "cl",     opcodePlayGroup, opcodePlayGroupExplicit);
            extensions.registerInstruction
                ("skipanim",              "",       opcodeSkipAnim, opcodeSkipAnimExplicit);
        }
    }

    namespace Tes4Combat
    {
        void registerExtensions (Compiler::Extensions& extensions)
        {
            extensions.registerFunction
                ("getattacked",           'l', "",  opcodeGetAttacked, opcodeGetAttackedExplicit);
            extensions.registerFunction
                ("getcombattarget",       'l', "",  opcodeGetCombatTarget, opcodeGetCombatTargetExplicit); // ref
            extensions.registerFunction
                ("getfriendhit",          'l', "c", opcodeGetFriendHit, opcodeGetFriendHitExplicit);
            extensions.registerFunction
                ("getignorefriendlyhits", 'l', "",  opcodeGetIgnoreFriendlyHits, opcodeGetIgnoreFriendlyHitsExplicit);
            extensions.registerFunction
                ("gifh",                  'l', "",  opcodeGetIgnoreFriendlyHits, opcodeGetIgnoreFriendlyHitsExplicit);
            extensions.registerFunction
                ("getshouldattack",       'l', "c", opcodeGetShouldAttack, opcodeGetShouldAttackExplicit);
            // See Actor state functions for IsInCombat
            extensions.registerInstruction
                ("setcombatstyle",        "/c",     opcodeSetCombatStyle, opcodeSetCombatStyleExplicit);
            extensions.registerInstruction
                ("setcs",                 "/c",     opcodeSetCombatStyle, opcodeSetCombatStyleExplicit);
            extensions.registerInstruction
                ("setignorefriendlyhits", "l",      opcodeSetIgnoreFriendlyHits, opcodeSetIgnoreFriendlyHitsExplicit);
            extensions.registerInstruction
                ("sifh",                  "l",      opcodeSetIgnoreFriendlyHits, opcodeSetIgnoreFriendlyHitsExplicit);
            extensions.registerInstruction
                ("startcombat",           "c",      opcodeStartCombat, opcodeStartCombatExplicit);
            // https://en.uesp.net/wiki/Tes4Mod:Script_Functions indicates no arguments, but
            // ArenaAggressionScript supplies "Player"
            extensions.registerInstruction
                ("stopcombat",            "/c",     opcodeStopCombat, opcodeStopCombatExplicit);
            extensions.registerInstruction
                ("stopcombatalarmonactor", "",      opcodeSCAOnActor, opcodeSCAOnActorExplicit);
            extensions.registerInstruction
                ("scaonactor",            "",       opcodeSCAOnActor, opcodeSCAOnActorExplicit);
            extensions.registerInstruction
                ("yield",                 "",       opcodeYield, opcodeYieldExplicit);
        }
    }

    namespace Tes4Crime
    {
        void registerExtensions (Compiler::Extensions& extensions)
        {
            extensions.registerFunction
                ("canpaycrimegold",       'l', "",  opcodeCanPayCrimeGold, opcodeCanPayCrimeGoldExplicit);
            extensions.registerFunction
                ("getalarmed",            'l', "",  opcodeGetAlarmed, opcodeGetAlarmedExplicit);
            extensions.registerFunction
                ("getcrime",              'l', "c/l", opcodeGetCrime, opcodeGetCrimeExplicit);
            extensions.registerFunction
                ("getcrimegold",          'f', "",  opcodeGetCrimeGold, opcodeGetCrimeGoldExplicit);
            extensions.registerFunction
                ("getcrimeknown",         'l', "lcc", opcodeGetCrimeKnown, opcodeGetCrimeKnownExplicit);
            extensions.registerFunction
                ("getdetectionlevel",     'l', "c", opcodeGetDetectionLevel, opcodeGetDetectionLevelExplicit);
            extensions.registerFunction
                ("gettrespasswarninglevel", 'l', "", opcodeGetTrespassWarningLevel, opcodeGetTrespassWarningLevelExplicit);
            extensions.registerInstruction
                ("gotojail",              "",       opcodeGoToJail, opcodeGoToJailExplicit);
            extensions.registerFunction
                ("isactordetected",       'l', "",  opcodeIsActorDetected, opcodeIsActorDetectedExplicit);
            extensions.registerFunction
                ("iscellowner",           'l', "c/c", opcodeIsCellOwner, opcodeIsCellOwnerExplicit);
            // See Actor state functions for IsGuard
            extensions.registerFunction
                ("ishorsestolen",         'l', "",  opcodeIsHorseStolen, opcodeIsHorseStolenExplicit);
            extensions.registerFunction
                ("isinmyownedcell",       'l', "",  opcodeIsInMyOwnedCell, opcodeIsInMyOwnedCellExplicit);
            // See Player functions for IsPCAMurderer
            // See Player functions for IsPlayerInJail
            extensions.registerFunction
                ("istrespassing",         'l', "",  opcodeIsTrespassing, opcodeIsTrespassingExplicit);
            extensions.registerInstruction
                ("killactor",             "/c",     opcodeKillActor, opcodeKillActorExplicit);
            extensions.registerInstruction
                ("kill",                  "/c",     opcodeKillActor, opcodeKillActorExplicit);
            extensions.registerInstruction
                ("modcrimegold",          "l",      opcodeModCrimeGold, opcodeModCrimeGoldExplicit);
            extensions.registerInstruction
                ("payfine",               "",       opcodePayFine, opcodePayFineExplicit);
            extensions.registerInstruction
                ("payfinethief",          "",       opcodePayFineThief, opcodePayFineThiefExplicit);
            extensions.registerInstruction
                ("sendtrespassalarm",     "c",      opcodeSendTrespassAlarm, opcodeSendTrespassAlarmExplicit);
            extensions.registerInstruction
                ("setcellownership",      "c/c",    opcodeSetCellOwnership, -1);
            extensions.registerInstruction
                ("setcellpublicflag",     "cl",     opcodeSetCellPublicFlag, -1);
            extensions.registerInstruction
                ("setpublic",             "cl",     opcodeSetCellPublicFlag, -1);
            extensions.registerInstruction
                ("setcrimegold",          "l",      opcodeSetCrimeGold, opcodeSetCrimeGoldExplicit);
        }
    }

    namespace Tes4Dialogue
    {
        void registerExtensions (Compiler::Extensions& extensions)
        {
            extensions.registerInstruction
                ("addtopic",              "c",      opcodeAddTopic, -1);
            extensions.registerInstruction
                ("refreshtopiclist",      "",       opcodeRefreshTopicList, -1);
            extensions.registerFunction
                ("say",                   'l', "c/lcll", opcodeSay, opcodeSayExplicit);
            extensions.registerFunction
                ("sayto",                 'l', "cc/l", opcodeSayTo, opcodeSayToExplicit);
            extensions.registerInstruction
                ("setnorumors",           "l",      opcodeSetNoRumors, opcodeSetNoRumorsExplicit);
            extensions.registerInstruction
                ("showdialogsubtitles",   "/l",     opcodeShowDialogSubtitles, -1);
            extensions.registerInstruction
                ("startconversation",     "c/c",    opcodeStartConversation, opcodeStartConversationExplicit);
        }
    }

    namespace Tes4Faction
    {
        void registerExtensions (Compiler::Extensions& extensions)
        {
            extensions.registerFunction
                ("getfactionrank",        'l', "c", opcodeGetFactionRank, opcodeGetFactionRankExplicit);
            extensions.registerFunction
                ("getfactionrankdifference", 'l', "cc", opcodeGetFactionRankDifference, opcodeGetFactionRankDifferenceExplicit);
            extensions.registerFunction
                ("getfactionreaction",    'l', "cc", opcodeGetFactionReaction, -1);
            extensions.registerFunction
                ("getinfaction",          'l', "c", opcodeGetInFaction, opcodeGetInFactionExplicit);
            // See Player functions for GetPCExpelled
            // See Player functions for GetPCFactionAttack
            // See Player functions for GetPCFactionMurder
            // See Player functions for GetPCFactionSteal
            // See Player functions for GetPCFactionSubmitAuthority
            extensions.registerInstruction
                ("modfactionrank",        "cl",     opcodeModFactionRank, opcodeModFactionRankExplicit);
            extensions.registerInstruction
                ("modfactionreaction",    "ccl",    opcodeModFactionReaction, -1);
            // See Actor state functions for SameFaction
            // See Actor state functions for SameFactionAsPC
            extensions.registerInstruction
                ("setfactionrank",        "cl",     opcodeSetFactionRank, opcodeSetFactionRankExplicit);
            extensions.registerInstruction
                ("setfactionreaction",    "ccl",    opcodeSetFactionReaction, -1);
            // See Player functions for SetPCExpelled
            // See Player functions for SetPCFactionAttack
            // See Player functions for SetPCFactionMurder
            // See Player functions for SetPCFactionSteal
            // See Player functions for SetPCFactionSubmitAuthority
        }
    }

    namespace Tes4Inventory
    {
        void registerExtensions (Compiler::Extensions& extensions)
        {
            extensions.registerInstruction
                ("activate",              "/cl",    opcodeActivate, opcodeActivateExplicit);
            extensions.registerInstruction
                ("addflames",             "",       opcodeAddFlames, opcodeAddFlamesExplicit);
            extensions.registerInstruction
                ("additem",               "cl",     opcodeAddItem, opcodeAddItemExplicit);
            extensions.registerFunction
                ("canhaveflames",         'l', "",  opcodeCanHaveFlames, opcodeCanHaveFlamesExplicit);
            extensions.registerInstruction
                ("clearownership",        "",       opcodeClearOwnership, opcodeClearOwnershipExplicit);
            extensions.registerInstruction
                ("drop",                  "/cl",    opcodeDrop, opcodeDropExplicit);
            extensions.registerInstruction
                ("dropme",                "",       opcodeDropMe, opcodeDropMeExplicit);
            extensions.registerInstruction
                ("duplicateallitems",     "/cl",    opcodeDuplicateAllItems, opcodeDuplicateAllItemsExplicit);
            extensions.registerInstruction
                ("equipitem",             "c/l",    opcodeEquipItem, opcodeEquipItemExplicit);
            extensions.registerInstruction
                ("equipobject",           "c/l",    opcodeEquipItem, opcodeEquipItemExplicit);
            extensions.registerFunction
                ("getdisabled",           'l', "",  opcodeGetDisabled, opcodeGetDisabledExplicit);
            extensions.registerFunction
                ("getequipped",           'l', "c", opcodeGetEquipped, opcodeGetEquippedExplicit);
            extensions.registerFunction
                ("getfurnituremarkerid",  'l', "",  opcodeGetFurnitureMarkerID, opcodeGetFurnitureMarkerIDExplicit);
            extensions.registerFunction
                ("getisreference",        'l', "c", opcodeGetIsReference, opcodeGetIsReferenceExplicit); // why exp?
            extensions.registerFunction
                ("getitemcount",          'l', "c", opcodeGetItemCount, opcodeGetItemCountExplicit);
            extensions.registerFunction
                ("hasflames",             'l', "",  opcodeHasFlames, opcodeHasFlamesExplicit);
            extensions.registerFunction
                ("isowner",               'l', "/c", opcodeIsOwner, opcodeIsOwnerExplicit);
            extensions.registerInstruction
                ("removeallitems",        "/cl",    opcodeRemoveAllItems, opcodeRemoveAllItemsExplicit);
            extensions.registerInstruction
                ("removeflames",          "",       opcodeRemoveFlames, opcodeRemoveFlamesExplicit);
            extensions.registerInstruction
                ("removeitem",            "cl",     opcodeRemoveItem, opcodeRemoveItemExplicit);
            extensions.registerInstruction
                ("removeme",              "/c",     opcodeRemoveMe, opcodeRemoveMeExplicit);
            extensions.registerInstruction
                ("setownership",          "/c",     opcodeSetOwnership, opcodeSetOwnershipExplicit);
            extensions.registerInstruction
                ("setquestobject",        "cl",     opcodeSetQuestObject, -1);
            extensions.registerInstruction
                ("setrigidbodymass",      "f",      opcodeSetRigidBodyMass, -1);
            extensions.registerInstruction
                ("setshowquestitems",     "l",      opcodeSetShowQuestItems, -1);
            extensions.registerInstruction
                ("unequipitem",           "c/l",    opcodeUnequipItem, opcodeUnequipItemExplicit);
            extensions.registerInstruction
                ("unequipobject",         "c/l",    opcodeUnequipItem, opcodeUnequipItemExplicit);
        }
    }

    namespace Tes4Magic
    {
        void registerExtensions (Compiler::Extensions& extensions)
        {
            extensions.registerInstruction
                ("addspell",              "c",      opcodeAddSpell, opcodeAddSpellExplicit);
            extensions.registerInstruction
                ("cast",                  "cc",     opcodeCast, opcodeCastExplicit);
            extensions.registerInstruction
                ("dispel",                "c",      opcodeDispel, opcodeDispelExplicit);
            extensions.registerInstruction
                ("dispelallspells",       "",       opcodeDispelAllSpells, opcodeDispelAllSpellsExplicit);
            extensions.registerFunction
                ("hasmagiceffect",        'l', "c", opcodeHasMagicEffect, opcodeHasMagicEffectExplicit);
            extensions.registerFunction
                ("isspelltarget",         'l', "c", opcodeIsSpellTarget, opcodeIsSpellTargetExplicit);
            extensions.registerInstruction
                ("playmagiceffectvisuals", "c/f",   opcodePlayMagicEffectVisuals, opcodePlayMagicEffectVisualsExplicit);
            extensions.registerInstruction
                ("pme",                    "c/f",   opcodePlayMagicEffectVisuals, opcodePlayMagicEffectVisualsExplicit);
            extensions.registerInstruction
                ("playmagicshadervisuals", "c/f",   opcodePlayMagicShaderVisuals, opcodePlayMagicShaderVisualsExplicit);
            extensions.registerInstruction
                ("pms",                    "c/f",   opcodePlayMagicShaderVisuals, opcodePlayMagicShaderVisualsExplicit);
            extensions.registerInstruction
                ("preloadmagiceffect",    "c",      opcodePreloadMagicEffect, opcodePreloadMagicEffectExplicit);
            extensions.registerInstruction
                ("removespell",           "c",      opcodeRemoveSpell, opcodeRemoveSpellExplicit);
            extensions.registerFunction
                ("scripteffectelapsedseconds", 'l', "", opcodeScriptEffectElapsedSeconds, -1);
            extensions.registerInstruction
                ("selectplayerspell",     "c",      opcodeSelectPlayerSpell, -1);
            extensions.registerInstruction
                ("spspell",               "c",      opcodeSelectPlayerSpell, -1);
            extensions.registerInstruction
                ("setactoralpha",         "f",      opcodeSetActorAlpha, opcodeSetActorAlphaExplicit);
            extensions.registerInstruction
                ("saa",                   "f",      opcodeSetActorAlpha, opcodeSetActorAlphaExplicit);
            extensions.registerInstruction
                ("setactorrefraction",    "f",      opcodeSetActorRefraction, opcodeSetActorRefractionExplicit);
            extensions.registerInstruction
                ("sar",                   "f",      opcodeSetActorRefraction, opcodeSetActorRefractionExplicit);
            // See Player functions for ShowEnchantment
            // See Player functions for ShowSpellMaking
            extensions.registerInstruction
                ("stopmagiceffectvisuals", "c",     opcodeStopMagicEffectVisuals, opcodeStopMagicEffectVisualsExplicit);
            extensions.registerInstruction
                ("sme",                    "c",     opcodeStopMagicEffectVisuals, opcodeStopMagicEffectVisualsExplicit);
            extensions.registerInstruction
                ("stopmagicshadervisuals", "c",     opcodeStopMagicShaderVisuals, opcodeStopMagicShaderVisualsExplicit);
            extensions.registerInstruction
                ("sms",                    "c",     opcodeStopMagicShaderVisuals, opcodeStopMagicShaderVisualsExplicit);
        }
    }

    namespace Tes4Movement
    {
        void registerExtensions (Compiler::Extensions& extensions)
        {
            extensions.registerFunction
                ("getangle",              'f', "c", opcodeGetAngle, opcodeGetAngleExplicit);
            extensions.registerFunction
                ("getdistance",           'f', "c", opcodeGetDistance, opcodeGetDistanceExplicit);
            extensions.registerFunction
                ("getheadingangle",       'f', "c", opcodeGetHeadingAngle, opcodeGetHeadingAngleExplicit);
            extensions.registerFunction
                ("getlineofsight",        'l', "c", opcodeGetLineOfSight, opcodeGetLineOfSightExplicit);
            extensions.registerFunction
                ("getlos",                'l', "c", opcodeGetLineOfSight, opcodeGetLineOfSightExplicit);
            extensions.registerFunction
                ("getpos",                'f', "c", opcodeGetPos, opcodeGetPosExplicit);
            extensions.registerFunction
                ("getstartingangle",      'f', "c", opcodeGetStartingAngle, opcodeGetStartingAngleExplicit);
            extensions.registerFunction
                ("getstartingpos",        'f', "c", opcodeGetStartingPos, opcodeGetStartingPosExplicit);
            extensions.registerInstruction
                ("movetomarker",          "c/fff",  opcodeMoveToMarker, opcodeMoveToMarkerExplicit);
            extensions.registerInstruction
                ("moveto",                "c/fff",  opcodeMoveToMarker, opcodeMoveToMarkerExplicit);
            extensions.registerInstruction
                ("positioncell",          "ffffc",  opcodePositionCell, opcodePositionCellExplicit);
            extensions.registerInstruction
                ("poscell",               "ffffc",  opcodePositionCell, opcodePositionCellExplicit);
            extensions.registerInstruction
                ("positionworld",         "ffffc",  opcodePositionWorld, opcodePositionWorldExplicit);
            extensions.registerInstruction
                ("posworld",              "ffffc",  opcodePositionWorld, opcodePositionWorldExplicit);
            extensions.registerInstruction
                ("rotate",                "cl",     opcodeRotate, opcodeRotateExplicit);
            extensions.registerInstruction
                ("setangle",              "cf",     opcodeSetAngle, opcodeSetAngleExplicit); // integer?
            extensions.registerInstruction
                ("setpos",                "cf",     opcodeSetPos, opcodeSetPosExplicit);
        }
    }

    namespace Tes4Player
    {
        void registerExtensions (Compiler::Extensions& extensions)
        {
            extensions.registerInstruction
                ("advancepclevel",        "",       opcodeAdvancePCLevel, -1);
            extensions.registerInstruction
                ("advlevel",              "",       opcodeAdvancePCLevel, -1);
            extensions.registerInstruction
                ("advancepcskill",        "cl",     opcodeAdvancePCSkill, -1);
            extensions.registerInstruction
                ("advskill",              "cl",     opcodeAdvancePCSkill, -1);
            extensions.registerInstruction
                ("disableplayercontrols", "",       opcodeDisablePlayerControls, -1);
            extensions.registerInstruction
                ("enableplayercontrols",  "",       opcodeEnablePlayerControls, -1);
            extensions.registerFunction
                ("getamountsoldstolen",   'l', "",  opcodeGetAmountSoldStolen, -1);
            extensions.registerFunction
                ("getclassdefaultmatch",  'l', "",  opcodeGetClassDefaultMatch, -1);
            extensions.registerFunction
                ("getisclassdefault",     'l', "c", opcodeGetIsClassDefault, -1);
            extensions.registerFunction
                ("getisplayerbirthsign",  'l', "c", opcodeGetIsPlayerBirthsign, -1);
            extensions.registerFunction
                ("getpbs",                'l', "c", opcodeGetIsPlayerBirthsign, -1);
            extensions.registerFunction
                ("getpcexpelled",         'l', "c", opcodeGetPCExpelled, -1);
            extensions.registerFunction
                ("getpcfactionattack",    'l', "c", opcodeGetPCFactionAttack, -1);
            extensions.registerFunction
                ("getpcfactionmurder",    'l', "c", opcodeGetPCFactionMurder, -1);
            extensions.registerFunction
                ("getpcfactionsteal",     'l', "c", opcodeGetPCFactionSteal, -1);
            extensions.registerFunction
                ("getpcfactionsubmitauthority", 'l', "c", opcodeGetPCFactionSubmitAuthority, -1);
            extensions.registerFunction
                ("getpcfame",             'l', "",  opcodeGetPCFame, -1);
            extensions.registerFunction
                ("getpcinfaction",        'l', "c", opcodeGetPCInFaction, -1);
            extensions.registerFunction
                ("getpcinfamy",           'l', "",  opcodeGetPCInfamy, -1);
            extensions.registerFunction
                ("getpcisclass",          'l', "c", opcodeGetPCIsClass, -1);
            extensions.registerFunction
                ("getpcisrace",           'l', "c", opcodeGetPCIsRace, -1);
            extensions.registerFunction
                ("getpcissex",            'l', "c", opcodeGetPCIsSex, -1);
            extensions.registerFunction
                ("getpcmiscstat",         'l', "l", opcodeGetPCMiscStat, -1);
            extensions.registerFunction
                ("getpcms",               'l', "l", opcodeGetPCMiscStat, -1);
            extensions.registerFunction
                ("getpcsleephours",       'l', "",  opcodeGetPCSleepHours, -1);
            extensions.registerFunction
                ("getplayercontrolsdisabled", 'l', "", opcodeGetPlayerControlsDisabled, -1);
            extensions.registerFunction
                ("getplayerhaslastriddenhorse", 'l', "", opcodeGetPlayerHasLastRiddenHorse, -1);
            extensions.registerFunction
                ("getplayerinseworld",    'l', "",  opcodeGetPlayerInSEWorld, -1);
            extensions.registerFunction
                ("hasvampirefed",         'l', "",  opcodeHasVampireFed, opcodeHasVampireFedExplicit);
            extensions.registerFunction
                ("ispcamurderer",         'l', "",  opcodeIsPCAMurderer, -1);
            extensions.registerFunction
                ("ispcsleeping",          'l', "",  opcodeIsPCSleeping, -1);
            extensions.registerFunction
                ("isplayerinjail",        'l', "",  opcodeIsPlayerInJail, -1);
            extensions.registerFunction
                ("isplayermovingintonewspace", 'l', "", opcodeIsPlayerMovingIntoNewSpace, -1);
            extensions.registerFunction
                ("isplayerslastriddenhorse", 'l', "", opcodeIsPlayersLastRiddenHorse, opcodeIsPlayersLastRiddenHorseExplicit);
            extensions.registerFunction
                ("istimepassing",         'l', "",  opcodeIsTimePassing, -1);
            extensions.registerFunction
                ("isturnarrest",          'l', "",  opcodeIsTurnArrest, -1);
            extensions.registerInstruction
                ("modamountsoldstolen",   "l",      opcodeModAmountSoldStolen, -1);
            extensions.registerInstruction
                ("modpcattribute",        "cl",     opcodeModPCAttribute, -1);
            extensions.registerInstruction
                ("modpca",                "cl",     opcodeModPCAttribute, -1);
            extensions.registerInstruction
                ("modpcfame",             "l",      opcodeModPCFame, -1);
            extensions.registerInstruction
                ("modpcinfamy",           "l",      opcodeModPCInfamy, -1);
            extensions.registerInstruction
                ("modpcmiscstat",         "ll",     opcodeModPCMiscStat, -1);
            extensions.registerInstruction
                ("modpcms",               "ll",     opcodeModPCMiscStat, -1);
            extensions.registerInstruction
                ("modpcskill",            "cl",     opcodeModPCSkill, -1);
            extensions.registerInstruction
                ("modpcs",                "cl",     opcodeModPCSkill, -1);
            extensions.registerInstruction
                ("setinchargen",          "l",      opcodeSetInChargen, -1);
            extensions.registerInstruction
                ("setpcexpelled",         "cl",     opcodeSetPCExpelled, -1);
            extensions.registerInstruction
                ("setpcfactionattack",    "cl",     opcodeSetPCFactionAttack, -1);
            extensions.registerInstruction
                ("setpcfactionmurder",    "cl",     opcodeSetPCFactionMurder, -1);
            extensions.registerInstruction
                ("setpcfactionsteal",     "cl",     opcodeSetPCFactionSteal, -1);
            extensions.registerInstruction
                ("setpcfactionsubmitauthority", "cl", opcodeSetPCFactionSubmitAuthority, -1);
            extensions.registerInstruction
                ("setpcfame",              "l",     opcodeSetPCFame, -1);
            extensions.registerInstruction
                ("setpcinfamy",            "l",     opcodeSetPCInfamy, -1);
            extensions.registerInstruction
                ("setpcsleephours",        "l",     opcodeSetPCSleepHours, -1);
            extensions.registerInstruction
                ("setplayerinseworld",     "l",     opcodeSetPlayerInSEWorld, -1);
            extensions.registerInstruction
                ("showbirthsignmenu",      "",      opcodeShowBirthSignMenu, -1);
            extensions.registerInstruction
                ("showclassmenu",          "",      opcodeShowClassMenu, -1);
            extensions.registerInstruction
                ("showenchantment",       "",       opcodeShowEnchantment, -1);
            extensions.registerInstruction
                ("showsracemenu",         "",       opcodeShowRaceMenu, -1);
            extensions.registerInstruction
                ("showspellmaking",       "",       opcodeShowSpellMaking, -1);
            extensions.registerInstruction
                ("triggerhitshader",      "/f",     opcodeTriggerHitShader, -1);
            extensions.registerInstruction
                ("ths",                   "/f",     opcodeTriggerHitShader, -1);
            extensions.registerInstruction
                ("wakeuppc",              "/l",     opcodeWakeUpPC, -1);
        }
    }

    namespace Tes4Quest
    {
        void registerExtensions (Compiler::Extensions& extensions)
        {
            extensions.registerInstruction
                ("completequest",         "c",      opcodeCompleteQuest, -1);
            extensions.registerFunction
                ("getquestrunning",       'l', "c", opcodeGetQuestRunning, -1);
            extensions.registerFunction
                ("getqr",                 'l', "c", opcodeGetQuestRunning, -1);
            extensions.registerFunction
                ("getquestvariable",      'f', "cc", opcodeGetQuestVariable, -1); // some are integer?
            extensions.registerFunction
                ("getstage",              'l', "c", opcodeGetStage, -1);
            extensions.registerFunction
                ("getstagedone",          'l', "cl", opcodeGetStageDone, -1);
            extensions.registerInstruction
                ("setstage",              "cl",     opcodeSetStage, -1);
            extensions.registerInstruction
                ("startquest",            "c",      opcodeStartQuest, -1);
            extensions.registerInstruction
                ("stopquest",             "c",      opcodeStopQuest, -1);
        }
    }

    namespace Tes4Statistics
    {
        void registerExtensions (Compiler::Extensions& extensions)
        {
            // See Actor value functions for ForceActorValue
            // See Actor value functions for GetActorValue
            extensions.registerFunction
                ("getarmorrating",        'l', "",  opcodeGetArmorRating, opcodeGetArmorRatingExplicit);
            extensions.registerFunction
                ("getarmorratingupperbody", 'l', "", opcodeGetArmorRatingUpperBody, opcodeGetArmorRatingUpperBodyExplicit);
            extensions.registerFunction
                ("getbartergold",         'l', "",  opcodeGetBarterGold, opcodeGetBarterGoldExplicit);
            // See Actor value functions for GetBaseActorValue
            // See Actor value functions for GetDisposition
            extensions.registerFunction
                ("getfatiguepercentage",  'l', "",  opcodeGetFatiguePercentage, opcodeGetGetFatiguePercentagelicit);
            extensions.registerFunction
                ("getfatigue",            'l', "",  opcodeGetFatiguePercentage, opcodeGetGetFatiguePercentagelicit);
            extensions.registerFunction
                ("getgold",               'l', "",  opcodeGetGold, opcodeGetGoldExplicit);
            extensions.registerFunction
                ("getinvestmentgold",     'l', "",  opcodeGetInvestmentGold, opcodeGetInvestmentGoldExplicit);
            extensions.registerFunction
                ("getlevel",              'l', "",  opcodeGetLevel, opcodeGetLevelExplicit);
            extensions.registerFunction
                ("getscale",              'l', "",  opcodeGetScale, opcodeGetScaleExplicit);
            extensions.registerFunction
                ("getwalkspeed",          'l', "",  opcodeGetWalkSpeed, opcodeWalkSpeedExplicit);
            extensions.registerFunction
                ("getwalk",               'l', "",  opcodeGetWalkSpeed, opcodeWalkSpeedExplicit);
            // See Actor value functions for ModActorValue
            extensions.registerFunction
                ("getmodbartergold",      'l', "l", opcodeModBarterGold, opcodeModBarterGoldExplicit);
            // See Actor value functions for ModDisposition
            extensions.registerFunction
                ("getmodscale",           'l', "f", opcodeModScale, opcodeModScaleExplicit);
            // See Actor value functions for ResetHealth
            // See Actor value functions for Resurrect
            // See Actor value functions for SetActorValue
            extensions.registerFunction
                ("getsetbartergold",      'l', "l", opcodeSetBarterGold, opcodeSetBarterGoldExplicit);
            // See Actor functions for SetEssential
            extensions.registerInstruction
                ("setinvestmentgold",     "l",      opcodeSetInvestmentGold, opcodeSetInvestmentGoldExplicit);
            extensions.registerInstruction
                ("setitemvalue",          "l",      opcodeSetItemValue, opcodeSetItemValueExplicit);
            extensions.registerInstruction
                ("setscale",              "f",      opcodeSetScale, opcodeSetScaleExplicit);
        }
    }

    namespace Tes4Weather
    {
        void registerExtensions (Compiler::Extensions& extensions)
        {
            extensions.registerInstruction
                ("forceweather",          "c/l",    opcodeForceWeather, -1);
            extensions.registerInstruction
                ("fw",                    "c/l",    opcodeForceWeather, -1);
            extensions.registerFunction
                ("getcurrentweatherpercent", 'l', "", opcodeGetCurrentWeatherPercent, -1);
            extensions.registerFunction
                ("getweatherpct",         'l', "",  opcodeGetCurrentWeatherPercent, -1);
            extensions.registerFunction
                ("getiscurrentweather",   'l', "c", opcodeGetIsCurrentWeather, -1);
            extensions.registerFunction
                ("getweather",            'l', "c", opcodeGetIsCurrentWeather, -1);
            extensions.registerFunction
                ("getwindspeed",          'f', "",  opcodeGetWindSpeed, -1);
            extensions.registerFunction
                ("iscloudy",              'l', "",  opcodeIsCloudy, -1);
            extensions.registerFunction
                ("ispleasant",            'l', "",  opcodeIsPleasant, -1);
            extensions.registerFunction
                ("israining",             'l', "",  opcodeIsRaining, -1);
            extensions.registerFunction
                ("issnowing",             'l', "",  opcodeIsSnowing, -1);
            extensions.registerInstruction
                ("releaseweatheroverride", "",      opcodeReleaseWeatherOverride, -1);
            extensions.registerInstruction
                ("rwo",                    "",      opcodeReleaseWeatherOverride, -1);
            extensions.registerInstruction
                ("setweather",            "c/l",    opcodeSetWeather, -1);
            extensions.registerInstruction
                ("sw",                    "c/l",    opcodeSetWeather, -1);
        }
    }

    namespace Tes4Misc
    {
        void registerExtensions (Compiler::Extensions& extensions)
        {
            extensions.registerInstruction
                ("addachievement",        "l",      opcodeAddAchievement, -1);
            extensions.registerInstruction
                ("autosave",              "",       opcodeAutosave, -1);
            extensions.registerInstruction
                ("closecurrentobliviongate", "/l",  opcodeCloseCurrentOblivionGate, -1);
            extensions.registerInstruction
                ("closeobliviongate",     "/l",     opcodeCloseOblivionGate, opcodeCloseOblivionGateExplicit);
            extensions.registerInstruction
                ("disable",               "",       opcodeDisable, opcodeDisableExplicit);
            // has explicit despite https://en.uesp.net/wiki/Tes4Mod:Script_Functions
            // indicating it is not a RefFunc - see MQ16Script
            extensions.registerInstruction
                ("disablelinkedpathpoints", "",     opcodeDisableLinkedPathPoints, opcodeDisableLinkedPathPointsExplicit);
            extensions.registerInstruction
                ("enable",                "",       opcodeEnable, opcodeEnableExplicit);
            extensions.registerInstruction
                ("enablefasttravel",       "l",     opcodeEnableFastTravel, -1);
            extensions.registerInstruction
                ("enablefast",             "l",     opcodeEnableFastTravel, -1);
            extensions.registerInstruction
                ("enablelinkedpathpoints", "",      opcodeEnableLinkedPathPoints, -1);
            extensions.registerInstruction
                ("forcecloseobliviongate", "/l",    opcodeForceCloseOblivionGate, -1);
            extensions.registerInstruction
                ("essentialdeathreload",  "c",      opcodeEssentialDeathReload, -1);
            extensions.registerFunction
                ("getactionref",          'l', "cl", opcodeGetActionRef, opcodeGetActionRefExplicit); // ref
            extensions.registerFunction
                ("getar",                 'l', "cl", opcodeGetActionRef, opcodeGetActionRefExplicit); // ref
            extensions.registerFunction
                ("getbuttonpressed",      'l', "",  opcodeGetButtonPressed, -1);
            extensions.registerFunction
                ("getdestroyed",          'l', "",  opcodeGetDestroyed, opcodeGetDestroyedExplicit);
            extensions.registerFunction
                ("getdoordefaultopen",    'l', "",  opcodeGetDoorDefaultOpen, opcodeGetDoorDefaultOpenExplicit);
            extensions.registerFunction
                ("getgamesetting",        'f', "c", opcodeGetGameSetting, -1); // can they be integer?
            extensions.registerFunction
                ("getgs",                 'f', "c", opcodeGetGameSetting, -1); // can they be integer?
            extensions.registerFunction
                ("getglobalvalue",        'f', "c", opcodeGetGlobalValue, -1); // can be integer?
            extensions.registerFunction
                ("getidledoneonce",       'l', "",  opcodeGetIdleDoneOnce, -1);
            extensions.registerFunction
                ("getlocked",             'l', "",  opcodeGetLocked, opcodeGetLockedExplicit);
            extensions.registerFunction
                ("getlocklevel",          'l', "",  opcodeGetLockLevel, opcodeGetLockLevelExplicit);
            extensions.registerFunction
                ("getopenstate",          'l', "",  opcodeGetOpenState, opcodeGetOpenStateExplicit);
            extensions.registerFunction
                ("getpersuasionnumber",   'l', "",  opcodeGetPersuasionNumber, -1);
            extensions.registerFunction
                ("getrandompercent",      'l', "",  opcodeGetRandomPercent, -1);
            extensions.registerFunction
                ("getscriptvariable",     'f', "cc", opcodeGetScriptVariable, -1); // can be integer?
            extensions.registerFunction
                ("gettotalpersuasionnumber", 'l', "", opcodeGetTotalPersuasionNumber, -1);
            extensions.registerFunction
                ("isactionref",           'l', "c", opcodeIsActionRef, opcodeIsActionRefExplicit);
            extensions.registerFunction
                ("isxbox",                'l', "",  opcodeIsXBox, -1);
            // arguments are short, bool but use long here
            extensions.registerInstruction
                ("lock",                  "/ll",    opcodeLock, opcodeLockExplicit);
            extensions.registerInstruction
                ("placeatme",             "c/lll",  opcodePlaceAtMe, opcodePlaceAtMeExplicit);
            extensions.registerInstruction
                ("playbink",              "c/l",    opcodePlayBink, -1);
            // has explicit despite https://en.uesp.net/wiki/Tes4Mod:Script_Functions
            // indicating it is not a RefFunc - see SE08XedQuestScript
            extensions.registerInstruction
                ("playsound",             "c",      opcodePlaySound, opcodePlaySoundExplicit);
            extensions.registerInstruction
                ("playsound3d",           "c",      opcodePlaySound3D, -1);
            extensions.registerInstruction
                ("purgecellbuffers",      "",       opcodePurgeCellBuffers, -1);
            extensions.registerInstruction
                ("pcb",                   "",       opcodePurgeCellBuffers, -1);
            extensions.registerInstruction
                ("reset3dstate",          "",       opcodeReset3DState, -1);
            extensions.registerInstruction
                ("resetfalldamagetimer",  "",       opcodeResetFallDamageTimer, opcodeResetFallDamageTimerExplicit);
            extensions.registerInstruction
                ("resetinterior",         "c",      opcodeResetInterior, -1);
            extensions.registerInstruction
                ("setactorfullname",      "c",      opcodeSetActorFullName, opcodeSetActorFullNameExplicit);
            extensions.registerInstruction
                ("setcellfullname",       "cc",     opcodeSetCellFullName, -1);
            extensions.registerInstruction
                ("setdestroyed",          "l",      opcodeSetDestroyed, opcodeSetDestroyedExplicit);
            extensions.registerInstruction
                ("setdoordefaultopen",    "l",      opcodeSetDoorDefaultOpen, opcodeSetDoorDefaultOpenExplicit);
            extensions.registerInstruction
                ("setopenstate",          "l",      opcodeSetOpenState, opcodeSetOpenStateExplicit);
            extensions.registerInstruction
                ("setsize",               "f",      opcodeSetSize, opcodeSetSizeExplicit);
            //"CSize" <- really?
            extensions.registerInstruction
                ("showmap",               "c/l",    opcodeShowMap, -1);
            extensions.registerInstruction
                ("streammusic",           "c",      opcodeStreamMusic, -1);
            extensions.registerInstruction
                ("trapupdate",            "/f",     opcodeTrapUpdate, -1);
            extensions.registerInstruction
                ("unlock",                "/l",     opcodeUnLock, opcodeUnLockExplicit);
            extensions.registerInstruction
                ("whichservicemenu",      "",       opcodeWhichServiceMenu, -1);

            // record var
            extensions.registerFunction
                ("getcontainer",          'l', "",  opcodeGetContainer, opcodeGetContainerExplicit); // ref
            extensions.registerFunction
                ("getpackagetarget",      'l', "",  opcodeGetPackageTarget, opcodeGetPackageTargetExplicit);
            extensions.registerFunction
                ("getparentref",          'l', "",  opcodeGetParentRef, -1);
            extensions.registerFunction
                ("getself",               'l', "",  opcodeGetSelf, -1);
            extensions.registerFunction
                ("this",                  'l', "",  opcodeGetSelf, -1);

            // time
            extensions.registerFunction
                ("getcurrenttime",        'f', "",  opcodeGetCurrentTime, -1);
            extensions.registerFunction
                ("getdayofweek",          'l', "",  opcodeGetDayOfWeek, -1);
            extensions.registerFunction
                ("getsecondspassed",      'l', "",  opcodeGetSecondsPassed, -1);


            // unknown
            // opcodeStartMasterFileSeekData
            // opcodeDumpMasterFileSeekData
            // opcodeGetUsedItemActivate
            // opcodeGetUsedItemLevel
            // opcodeShowNameMenu
            // opcodeShowViewerStrings

            // deprecated
            // SetAtStart
            // GetDisease
        }
    }
}


