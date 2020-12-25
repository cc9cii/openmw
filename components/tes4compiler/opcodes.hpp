#ifndef TES4COMPILER_OPCODES_H
#define TES4COMPILER_OPCODES_H

namespace Tes4Compiler
{
    namespace Tes4Event
    {
        // functions with optional arguments need to have opcode range for segment 3
        const int opcodeGameMode            = 0x2000400;
        const int opcodeMenuMode            = 0x0020040; // int (opt)
        const int opcodeOnActivate          = 0x0020041; // actor (opt)
        const int opcodeOnActorEquip        = 0x2000401;
        const int opcodeOnActorUnequip      = 0x2000402;
        const int opcodeOnAdd               = 0x0020042; // container (opt)
        const int opcodeOnAlarm             = 0x0020043; // crime type, actor (opt)
        const int opcodeOnAlarmVictim       = 0x0020044; // crime type, actor (opt)
        const int opcodeOnDeath             = 0x0020045; // actor (opt)
        const int opcodeOnDrop              = 0x0020046; // container (opt)
        const int opcodeOnEquip             = 0x0020047; // container (opt)
        const int opcodeOnHit               = 0x0020048; // actor (opt)
        const int opcodeOnHitWith           = 0x0020049; // object (opt)
        const int opcodeOnKnockout          = 0x2000403;
        const int opcodeOnLoad              = 0x2000404;
        const int opcodeOnMagicEffectHit    = 0x002004a; // magic effect (opt)
        const int opcodeOnMurder            = 0x002004b; // actor (opt)
        const int opcodeOnPackageChange     = 0x2000405;
        const int opcodeOnPackageDone       = 0x2000406;
        const int opcodeOnPackageEnd        = 0x2000407;
        const int opcodeOnPackageStart      = 0x2000408;
        const int opcodeOnReset             = 0x2000409;
        const int opcodeOnSell              = 0x002004c; // actor (opt)
        const int opcodeOnStartCombat       = 0x002004d; // actor (opt)
        const int opcodeOnTrigger           = 0x002004e; // obj ref (opt)
        const int opcodeOnTriggerActor      = 0x002004f; // obj ref (opt)
        const int opcodeOnTriggerMob        = 0x0020050; // obj ref (opt)
        const int opcodeOnUnequip           = 0x0020051; // container (opt)
        const int opcodeScriptEffectFinish  = 0x200040a;
        const int opcodeScriptEffectStart   = 0x200040b;
        const int opcodeScriptEffectUpdate  = 0x200040c;
    }

    namespace Tes4Actor
    {
        const int opcodeCreateFullActorCopy                 = 0x200040d;
        const int opcodeCreateFullActorCopyExplicit         = 0x200040e;
        const int opcodeDeleteFullActorCopy                 = 0x200040f;
        const int opcodeDeleteFullActorCopyExplicit         = 0x2000410;
        const int opcodeDuplicateNPCStats                   = 0x0020052; //
        const int opcodeDuplicateNPCStatsExplicit           = 0x0020053; //
        const int opcodeIsActor                             = 0x2000411;
        const int opcodeIsActorExplicit                     = 0x2000412;
        const int opcodeIsActorsAIOff                       = 0x2000413;
        const int opcodeIsActorsAIOffExplicit               = 0x2000414;
        const int opcodeKillAllActors                       = 0x0020054; //
        const int opcodeSetEssential                        = 0x0020055; //
        const int opcodeToggleActorsAI                      = 0x2000415;
        const int opcodeToggleActorsAIExplicit              = 0x2000416;
        const int opcodeGetClothingValue                    = 0x2000417;
        const int opcodeGetClothingValueExplicit            = 0x2000418;
        const int opcodeGetDead                             = 0x2000419;
        const int opcodeGetDeadExplicit                     = 0x200041a;
        const int opcodeGetDeadCount                        = 0x200041b;
        const int opcodeGetDetected                         = 0x200041c;
        const int opcodeGetDetectedExplicit                 = 0x200041d;
        const int opcodeGetForceRun                         = 0x200041e;
        const int opcodeGetForceRunExplicit                 = 0x200041f;
        const int opcodeGetForceSneak                       = 0x2000420;
        const int opcodeGetForceSneakExplicit               = 0x2000421;
        const int opcodeGetInCell                           = 0x2000422;
        const int opcodeGetInCellExplicit                   = 0x2000423;
        const int opcodeGetInCellParam                      = 0x2000424;
        const int opcodeGetInSameCell                       = 0x2000425;
        const int opcodeGetInSameCellExplicit               = 0x2000426;
        const int opcodeGetInWorldspace                     = 0x2000427;
        const int opcodeGetInWorldspaceExplicit             = 0x2000428;
        const int opcodeGetIsAlerted                        = 0x2000429;
        const int opcodeGetIsAlertedExplicit                = 0x200042a;
        const int opcodeGetIsClass                          = 0x200042b;
        const int opcodeGetIsClassExplicit                  = 0x200042c;
        const int opcodeGetIsCreature                       = 0x200042d;
        const int opcodeGetIsCreatureExplicit               = 0x200042e;
        const int opcodeGetIsGhost                          = 0x200042f;
        const int opcodeGetIsGhostExplicit                  = 0x2000430;
        const int opcodeGetIsID                             = 0x2000431;
        const int opcodeGetIsIDExplicit                     = 0x2000432;
        const int opcodeGetIsPlayableRace                   = 0x2000433;
        const int opcodeGetIsPlayableRaceExplicit           = 0x2000434;
        const int opcodeGetIsRace                           = 0x2000435;
        const int opcodeGetIsRaceExplicit                   = 0x2000436;
        const int opcodeGetIsSex                            = 0x2000437;
        const int opcodeGetIsSexExplicit                    = 0x2000438;
        const int opcodeGetIsUsedItem                       = 0x2000439;
        const int opcodeGetIsUsedItemType                   = 0x200043a;
        const int opcodeGetKnockedState                     = 0x200043b;
        const int opcodeGetKnockedStateExplicit             = 0x200043c;
        const int opcodeGetNoRumors                         = 0x200043d;
        const int opcodeNoRumorsExplicit                    = 0x200043e;
        const int opcodeGetOffersServicesNow                = 0x200043f;
        const int opcodeOffersServicesNowExplicit           = 0x2000440;
        const int opcodeGetSitting                          = 0x2000441;
        const int opcodeGetSittingExplicit                  = 0x2000442;
        const int opcodeGetSleeping                         = 0x2000443;
        const int opcodeGetSleepingExplicit                 = 0x2000444;
        const int opcodeGetTalkedToPC                       = 0x2000445;
        const int opcodeGetTalkedToPCExplicit               = 0x2000446;
        const int opcodeGetTalkedToPCParam                  = 0x2000447;
        const int opcodeGetTimeDead                         = 0x2000448;
        const int opcodeGetTimeDeadExplicit                 = 0x2000449;
        const int opcodeGetVampire                          = 0x200044a;
        const int opcodeGetVampireExplicit                  = 0x200044b;
        const int opcodeGetWeaponAnimType                   = 0x200044c;
        const int opcodeGetWeaponAnimTypeExplicit           = 0x200044d;
        const int opcodeGetWeaponSkillType                  = 0x200044e;
        const int opcodeGetWeaponSkillTypeExplicit          = 0x200044f;
        const int opcodeIsActorAVictim                      = 0x2000450;
        const int opcodeIsActorAVictimExplicit              = 0x2000451;
        const int opcodeIsActorEvil                         = 0x2000452;
        const int opcodeIsActorEvilExplicit                 = 0x2000453;
        const int opcodeIsActorUsingATorch                  = 0x2000454;
        const int opcodeIsActorUsingATorchExplicit          = 0x2000455;
        const int opcodeIsCurrentFurnitureObj               = 0x2000456;
        const int opcodeIsCurrentFurnitureObjExplicit       = 0x2000457;
        const int opcodeIsCurrentFurnitureRef               = 0x2000458;
        const int opcodeIsCurrentFurnitureRefExplicit       = 0x2000459;
        const int opcodeIsEssential                         = 0x200045a;
        const int opcodeIsEssentialExplicit                 = 0x200045b;
        const int opcodeIsFacingUp                          = 0x200045c;
        const int opcodeIsFacingUpExplicit                  = 0x200045d;
        const int opcodeIsGuard                             = 0x200045e;
        const int opcodeIsGuardExplicit                     = 0x200045f;
        const int opcodeIsIdlePlaying                       = 0x2000460;
        const int opcodeIsIdlePlayingExplicit               = 0x2000461;
        const int opcodeIsInCombat                          = 0x2000462;
        const int opcodeIsInCombatExplicit                  = 0x2000463;
        const int opcodeIsInDangerousWater                  = 0x2000464;
        const int opcodeIsInDangerousWaterExplicit          = 0x2000465;
        const int opcodeIsInInterior                        = 0x2000466;
        const int opcodeIsInInteriorExplicit                = 0x2000467;
        const int opcodeIsLeftUp                            = 0x2000468;
        const int opcodeIsLeftUpExplicit                    = 0x2000469;
        const int opcodeIsRidingHorse                       = 0x200046a;
        const int opcodeIsRidingHorseExplicit               = 0x200046b;
        const int opcodeIsRunning                           = 0x200046c;
        const int opcodeIsRunningExplicit                   = 0x200046d;
        const int opcodeIsShieldOut                         = 0x200046e;
        const int opcodeIsShieldOutExplicit                 = 0x200046f;
        const int opcodeIsSneaking                          = 0x2000470;
        const int opcodeIsSneakingExplicit                  = 0x2000471;
        const int opcodeIsSwimming                          = 0x2000472;
        const int opcodeIsSwimmingExplicit                  = 0x2000473;
        const int opcodeIsTalking                           = 0x2000474;
        const int opcodeIsTalkingExplicit                   = 0x2000475;
        const int opcodeIsTorchOut                          = 0x2000476;
        const int opcodeIsTorchOutExplicit                  = 0x2000477;
        const int opcodeIsWaiting                           = 0x2000478;
        const int opcodeIsWaitingExplicit                   = 0x2000479;
        const int opcodeIsWeaponOut                         = 0x200047a;
        const int opcodeIsWeaponOutExplicit                 = 0x200047b;
        const int opcodeIsYielding                          = 0x200047c;
        const int opcodeIsYieldingExplicit                  = 0x200047d;
        const int opcodePickIdle                            = 0x200047e;
        const int opcodePickIdleExplicit                    = 0x200047f;
        const int opcodePushActorAway                       = 0x2000480;
        const int opcodePushActorAwayExplicit               = 0x2000481;
        const int opcodeSameFaction                         = 0x2000482;
        const int opcodeSameFactionExplicit                 = 0x2000483;
        const int opcodeSameFactionAsPC                     = 0x2000484;
        const int opcodeSameFactionAsPCExplicit             = 0x2000485;
        const int opcodeSameRace                            = 0x2000486;
        const int opcodeSameRaceExplicit                    = 0x2000487;
        const int opcodeSameRaceAsPC                        = 0x2000488;
        const int opcodeSameRaceAsPCExplicit                = 0x2000489;
        const int opcodeSameSex                             = 0x200048a;
        const int opcodeSameSexExplicit                     = 0x200048b;
        const int opcodeSameSexAsPC                         = 0x200048c;
        const int opcodeSameSexAsPCExplicit                 = 0x200048d;
        const int opcodeSetClass                            = 0x200048e;
        const int opcodeSetClassExplicit                    = 0x200048f;
        const int opcodeSetForceRun                         = 0x2000490;
        const int opcodeSetForceRunExplicit                 = 0x2000491;
        const int opcodeSetForceSneak                       = 0x2000492;
        const int opcodeSetForceSneakExplicit               = 0x2000493;
        const int opcodeSetGhost                            = 0x2000494;
        const int opcodeSetGhostExplicit                    = 0x2000495;
        const int opcodeSetLevel                            = 0x0020056; //
        const int opcodeSetLevelExplicit                    = 0x0020057; //
        const int opcodeForceActorValue                     = 0x2000496;
        const int opcodeForceActorValueExplicit             = 0x2000497;
        const int opcodeGetActorValue                       = 0x2000498;
        const int opcodeGetActorValueExplicit               = 0x2000499;
        const int opcodeGetBaseActorValue                   = 0x200049a;
        const int opcodeGetBaseActorValueExplicit           = 0x200049b;
        const int opcodeGetDisposition                      = 0x200049c;
        const int opcodeGetDispositionExplicit              = 0x200049d;
        const int opcodeModActorValue                       = 0x200049e;
        const int opcodeModActorValueExplicit               = 0x200049f;
        const int opcodeModDisposition                      = 0x20004a0;
        const int opcodeModDispositionExplicit              = 0x20004a1;
        const int opcodeResetHealth                         = 0x20004a2;
        const int opcodeResetHealthExplicit                 = 0x20004a3;
        const int opcodeResurrectActor                      = 0x0020058; //
        const int opcodeResurrectActorExplicit              = 0x0020059; //
        const int opcodeSetActorValue                       = 0x20004a4;
        const int opcodeSetActorValueExplicit               = 0x20004a5;
    }

    namespace Tes4AI
    {
        const int opcodeAddScriptPackage                    = 0x20004a6;
        const int opcodeAddScriptPackageExplicit            = 0x20004a7;
        const int opcodeEvaluatePackage                     = 0x20004a8;
        const int opcodeEvaluatePackageExplicit             = 0x20004a9;
        const int opcodeForceFlee                           = 0x002005a; //
        const int opcodeForceFleeExplicit                   = 0x002005b; //
        const int opcodeForceTakeCover                      = 0x20004aa;
        const int opcodeForceTakeCoverExplicit              = 0x20004ab;
        const int opcodeGetCurrentAIPackage                 = 0x20004ac;
        const int opcodeGetCurrentAIPackageExplicit         = 0x20004ad;
        const int opcodeGetCurrentAIProcedure               = 0x20004ae;
        const int opcodeGetCurrentAIProcedureExplicit       = 0x20004af;
        const int opcodeGetIsCurrentPackage                 = 0x20004b0;
        const int opcodeGetIsCurrentPackageExplicit         = 0x20004b1;
        const int opcodeGetRestrained                       = 0x20004b2;
        const int opcodeGetRestrainedExplicit               = 0x20004b3;
        const int opcodeGetUnconscious                      = 0x20004b4;
        const int opcodeGetUnconsciousExplicit              = 0x20004b5;
        const int opcodeIsContinuingPackagePCNear           = 0x20004b6;
        const int opcodeIsContinuingPackagePCNearExplicit   = 0x20004b7;
        const int opcodeLook                                = 0x20004b8;
        const int opcodeLookExplicit                        = 0x20004b9;
        const int opcodeRemoveScriptPackage                 = 0x20004ba;
        const int opcodeRemoveScriptPackageExplicit         = 0x20004bb;
        const int opcodeSetActorsAI                         = 0x20004bc;
        const int opcodeSetActorsAIExplicit                 = 0x20004bd;
        const int opcodeSetAlert                            = 0x20004be;
        const int opcodeSetAlertExplicit                    = 0x20004bf;
        const int opcodeSetAllReachable                     = 0x20004c0;
        const int opcodeSetAllVisible                       = 0x20004c1;
        const int opcodeSetNoAvoidance                      = 0x20004c2;
        const int opcodeSetPackDuration                     = 0x20004c3;
        const int opcodeSetPackDurationExplicit             = 0x20004c4;
        const int opcodeSetRestrained                       = 0x20004c5;
        const int opcodeSetRestrainedExplicit               = 0x20004c6; // unexpected
        const int opcodeSetSceneIsComplex                   = 0x20004c7;
        const int opcodeSetUnconscious                      = 0x20004c8;
        const int opcodeSetUnconsciousExplicit              = 0x20004c9; // unexpected
        const int opcodeStopLook                            = 0x002005c; //
        const int opcodeStopLookExplicit                    = 0x002005d; //
        const int opcodeStopWaiting                         = 0x20004ca;
        const int opcodeStopWaitingExplicit                 = 0x20004cb;
        const int opcodeVampireFeed                         = 0x20004cc;
        const int opcodeVampireFeedExplicit                 = 0x20004cd;
        const int opcodeWait                                = 0x20004ce;
        const int opcodeWaitExplicit                        = 0x20004cf;
    }

    namespace Tes4Animation
    {
        const int opcodeIsAnimPlaying                       = 0x20004d0;
        const int opcodeIsAnimPlayingExplicit               = 0x20004d1;
        const int opcodeLoopGroup                           = 0x20004d2;
        const int opcodeLoopGroupExplicit                   = 0x20004d3;
        const int opcodePlayGroup                           = 0x20004d4;
        const int opcodePlayGroupExplicit                   = 0x20004d5;
        const int opcodeSkipAnim                            = 0x20004d6;
        const int opcodeSkipAnimExplicit                    = 0x20004d7;
    }

    namespace Tes4Combat
    {
        const int opcodeGetAttacked                         = 0x20004d8;
        const int opcodeGetAttackedExplicit                 = 0x20004d9;
        const int opcodeGetCombatTarget                     = 0x20004da;
        const int opcodeGetCombatTargetExplicit             = 0x20004db;
        const int opcodeGetFriendHit                        = 0x20004dc;
        const int opcodeGetFriendHitExplicit                = 0x20004dd;
        const int opcodeGetIgnoreFriendlyHits               = 0x20004de;
        const int opcodeGetIgnoreFriendlyHitsExplicit       = 0x20004df;
        const int opcodeGetShouldAttack                     = 0x20004e0;
        const int opcodeGetShouldAttackExplicit             = 0x20004e1;
        const int opcodeSetCombatStyle                      = 0x002005e; //
        const int opcodeSetCombatStyleExplicit              = 0x002005f; //
        const int opcodeSetIgnoreFriendlyHits               = 0x20004e2;
        const int opcodeSetIgnoreFriendlyHitsExplicit       = 0x20004e3;
        const int opcodeStartCombat                         = 0x20004e4;
        const int opcodeStartCombatExplicit                 = 0x20004e5;
        const int opcodeStopCombat                          = 0x0020060; //
        const int opcodeStopCombatExplicit                  = 0x0020061; //
        const int opcodeSCAOnActor                          = 0x20004e6;
        const int opcodeSCAOnActorExplicit                  = 0x20004e7;
        const int opcodeYield                               = 0x20004e8;
        const int opcodeYieldExplicit                       = 0x20004e9;
    }

    namespace Tes4Crime
    {
        const int opcodeCanPayCrimeGold                     = 0x20004ea;
        const int opcodeCanPayCrimeGoldExplicit             = 0x20004eb;
        const int opcodeGetAlarmed                          = 0x20004ec;
        const int opcodeGetAlarmedExplicit                  = 0x20004ed;
        const int opcodeGetCrime                            = 0x0020062; //
        const int opcodeGetCrimeExplicit                    = 0x0020063; //
        const int opcodeGetCrimeGold                        = 0x20004ee;
        const int opcodeGetCrimeGoldExplicit                = 0x20004ef;
        const int opcodeGetCrimeKnown                       = 0x20004f0;
        const int opcodeGetCrimeKnownExplicit               = 0x20004f1;
        const int opcodeGetDetectionLevel                   = 0x20004f2;
        const int opcodeGetDetectionLevelExplicit           = 0x20004f3;
        const int opcodeGetTrespassWarningLevel             = 0x20004f4;
        const int opcodeGetTrespassWarningLevelExplicit     = 0x20004f5;
        const int opcodeGoToJail                            = 0x20004f6;
        const int opcodeGoToJailExplicit                    = 0x20004f7;
        const int opcodeIsActorDetected                     = 0x20004f8;
        const int opcodeIsActorDetectedExplicit             = 0x20004f9;
        const int opcodeIsCellOwner                         = 0x0020064; //
        const int opcodeIsCellOwnerExplicit                 = 0x0020065; //
        const int opcodeIsHorseStolen                       = 0x20004fa;
        const int opcodeIsHorseStolenExplicit               = 0x20004fb;
        const int opcodeIsInMyOwnedCell                     = 0x20004fc;
        const int opcodeIsInMyOwnedCellExplicit             = 0x20004fd;
        const int opcodeIsTrespassing                       = 0x20004fe;
        const int opcodeIsTrespassingExplicit               = 0x20004ff;
        const int opcodeKillActor                           = 0x0020066; //
        const int opcodeKillActorExplicit                   = 0x0020067; //
        const int opcodeModCrimeGold                        = 0x2000500;
        const int opcodeModCrimeGoldExplicit                = 0x2000501;
        const int opcodePayFine                             = 0x2000502;
        const int opcodePayFineExplicit                     = 0x2000503;
        const int opcodePayFineThief                        = 0x2000504;
        const int opcodePayFineThiefExplicit                = 0x2000505;
        const int opcodeSendTrespassAlarm                   = 0x2000506;
        const int opcodeSendTrespassAlarmExplicit           = 0x2000507;
        const int opcodeSetCellOwnership                    = 0x0020068; //
        const int opcodeSetCellPublicFlag                   = 0x2000508;
        const int opcodeSetCrimeGold                        = 0x2000509;
        const int opcodeSetCrimeGoldExplicit                = 0x200050a;
    }

    namespace Tes4Dialogue
    {
        const int opcodeAddTopic                            = 0x200050b;
        const int opcodeRefreshTopicList                    = 0x200050c;
        const int opcodeSay                                 = 0x0020069; //
        const int opcodeSayExplicit                         = 0x002006a; //
        const int opcodeSayTo                               = 0x002006b; //
        const int opcodeSayToExplicit                       = 0x002006c; //
        const int opcodeSetNoRumors                         = 0x200050d;
        const int opcodeSetNoRumorsExplicit                 = 0x200050e;
        const int opcodeShowDialogSubtitles                 = 0x002006d; //
        const int opcodeStartConversation                   = 0x200050f;
        const int opcodeStartConversationExplicit           = 0x2000510;
    }

    namespace Tes4Faction
    {
        const int opcodeGetFactionRank                      = 0x2000511;
        const int opcodeGetFactionRankExplicit              = 0x2000512;
        const int opcodeGetFactionRankDifference            = 0x2000513;
        const int opcodeGetFactionRankDifferenceExplicit    = 0x2000514;
        const int opcodeGetFactionReaction                  = 0x2000515;
        const int opcodeGetInFaction                        = 0x2000516;
        const int opcodeGetInFactionExplicit                = 0x2000517;
        const int opcodeModFactionRank                      = 0x2000518;
        const int opcodeModFactionRankExplicit              = 0x2000519;
        const int opcodeModFactionReaction                  = 0x200051a;
        const int opcodeSetFactionRank                      = 0x200051b;
        const int opcodeSetFactionRankExplicit              = 0x200051c;
        const int opcodeSetFactionReaction                  = 0x200051d;
    }

    namespace Tes4Inventory
    {
        const int opcodeActivate                            = 0x002006e; //
        const int opcodeActivateExplicit                    = 0x002006f; //
        const int opcodeAddFlames                           = 0x200051e;
        const int opcodeAddFlamesExplicit                   = 0x200051f;
        const int opcodeAddItem                             = 0x2000520;
        const int opcodeAddItemExplicit                     = 0x2000521;
        const int opcodeCanHaveFlames                       = 0x2000522;
        const int opcodeCanHaveFlamesExplicit               = 0x2000523;
        const int opcodeClearOwnership                      = 0x2000524;
        const int opcodeClearOwnershipExplicit              = 0x2000525;
        const int opcodeDrop                                = 0x0020070; //
        const int opcodeDropExplicit                        = 0x0020071; //
        const int opcodeDropMe                              = 0x2000526;
        const int opcodeDropMeExplicit                      = 0x2000527;
        const int opcodeDuplicateAllItems                   = 0x0020072; //
        const int opcodeDuplicateAllItemsExplicit           = 0x0020073; //
        const int opcodeEquipItem                           = 0x0020074; //
        const int opcodeEquipItemExplicit                   = 0x0020075; //
        const int opcodeGetDisabled                         = 0x2000528;
        const int opcodeGetDisabledExplicit                 = 0x2000529;
        const int opcodeGetEquipped                         = 0x200052a;
        const int opcodeGetEquippedExplicit                 = 0x200052b;
        const int opcodeGetFurnitureMarkerID                = 0x200052c;
        const int opcodeGetFurnitureMarkerIDExplicit        = 0x200052d;
        const int opcodeGetIsReference                      = 0x200052e;
        const int opcodeGetIsReferenceExplicit              = 0x200052f;
        const int opcodeGetItemCount                        = 0x2000530;
        const int opcodeGetItemCountExplicit                = 0x2000531;
        const int opcodeHasFlames                           = 0x2000532;
        const int opcodeHasFlamesExplicit                   = 0x2000533;
        const int opcodeIsOwner                             = 0x0020076; //
        const int opcodeIsOwnerExplicit                     = 0x0020077; //
        const int opcodeRemoveAllItems                      = 0x0020078; //
        const int opcodeRemoveAllItemsExplicit              = 0x0020079; //
        const int opcodeRemoveFlames                        = 0x2000534;
        const int opcodeRemoveFlamesExplicit                = 0x2000535;
        const int opcodeRemoveItem                          = 0x2000536;
        const int opcodeRemoveItemExplicit                  = 0x2000537;
        const int opcodeRemoveMe                            = 0x002007a; //
        const int opcodeRemoveMeExplicit                    = 0x002007b; //
        const int opcodeSetOwnership                        = 0x002007c; //
        const int opcodeSetOwnershipExplicit                = 0x002007d; //
        const int opcodeSetQuestObject                      = 0x2000538;
        const int opcodeSetRigidBodyMass                    = 0x2000539;
        const int opcodeSetShowQuestItems                   = 0x200053a;
        const int opcodeUnequipItem                         = 0x002007e; //
        const int opcodeUnequipItemExplicit                 = 0x002007f; //
    }

    namespace Tes4Magic
    {
        const int opcodeAddSpell                            = 0x200053b;
        const int opcodeAddSpellExplicit                    = 0x200053c;
        const int opcodeCast                                = 0x200053d;
        const int opcodeCastExplicit                        = 0x200053e;
        const int opcodeDispel                              = 0x200053f;
        const int opcodeDispelExplicit                      = 0x2000540;
        const int opcodeDispelAllSpells                     = 0x2000541;
        const int opcodeDispelAllSpellsExplicit             = 0x2000542;
        const int opcodeHasMagicEffect                      = 0x2000543;
        const int opcodeHasMagicEffectExplicit              = 0x2000544;
        const int opcodeIsSpellTarget                       = 0x2000545;
        const int opcodeIsSpellTargetExplicit               = 0x2000546;
        const int opcodePlayMagicEffectVisuals              = 0x0020080; //
        const int opcodePlayMagicEffectVisualsExplicit      = 0x0020081; //
        const int opcodePlayMagicShaderVisuals              = 0x0020082; //
        const int opcodePlayMagicShaderVisualsExplicit      = 0x0020083; //
        const int opcodePreloadMagicEffect                  = 0x2000547;
        const int opcodePreloadMagicEffectExplicit          = 0x2000548;
        const int opcodeRemoveSpell                         = 0x2000549;
        const int opcodeRemoveSpellExplicit                 = 0x200054a;
        const int opcodeScriptEffectElapsedSeconds          = 0x200054b;
        const int opcodeSelectPlayerSpell                   = 0x200054c;
        const int opcodeSetActorAlpha                       = 0x200054d;
        const int opcodeSetActorAlphaExplicit               = 0x200054e;
        const int opcodeSetActorRefraction                  = 0x200054f;
        const int opcodeSetActorRefractionExplicit          = 0x2000550;
        const int opcodeStopMagicEffectVisuals              = 0x2000551;
        const int opcodeStopMagicEffectVisualsExplicit      = 0x2000552;
        const int opcodeStopMagicShaderVisuals              = 0x2000553;
        const int opcodeStopMagicShaderVisualsExplicit      = 0x2000554;
    }

    namespace Tes4Movement
    {
        const int opcodeGetAngle                            = 0x2000555;
        const int opcodeGetAngleExplicit                    = 0x2000556;
        const int opcodeGetDistance                         = 0x2000557;
        const int opcodeGetDistanceExplicit                 = 0x2000558;
        const int opcodeGetHeadingAngle                     = 0x2000559;
        const int opcodeGetHeadingAngleExplicit             = 0x200055a;
        const int opcodeGetLineOfSight                      = 0x200055b;
        const int opcodeGetLineOfSightExplicit              = 0x200055c;
        const int opcodeGetPos                              = 0x200055d;
        const int opcodeGetPosExplicit                      = 0x200055e;
        const int opcodeGetStartingAngle                    = 0x200055f;
        const int opcodeGetStartingAngleExplicit            = 0x2000560;
        const int opcodeGetStartingPos                      = 0x2000561;
        const int opcodeGetStartingPosExplicit              = 0x2000562;
        const int opcodeMoveToMarker                        = 0x0020084; //
        const int opcodeMoveToMarkerExplicit                = 0x0020085; //
        const int opcodePositionCell                        = 0x2000563;
        const int opcodePositionCellExplicit                = 0x2000564;
        const int opcodePositionWorld                       = 0x2000565;
        const int opcodePositionWorldExplicit               = 0x2000566;
        const int opcodeRotate                              = 0x2000567;
        const int opcodeRotateExplicit                      = 0x2000568;
        const int opcodeSetAngle                            = 0x2000569;
        const int opcodeSetAngleExplicit                    = 0x200056a;
        const int opcodeSetPos                              = 0x200056b;
        const int opcodeSetPosExplicit                      = 0x200056c;
    }

    namespace Tes4Player
    {
        const int opcodeAdvancePCLevel                      = 0x200056d;
        const int opcodeAdvancePCSkill                      = 0x200056e;
        const int opcodeDisablePlayerControls               = 0x200056f;
        const int opcodeEnablePlayerControls                = 0x2000570;
        const int opcodeGetAmountSoldStolen                 = 0x2000571;
        const int opcodeGetClassDefaultMatch                = 0x2000572;
        const int opcodeGetIsClassDefault                   = 0x2000573;
        const int opcodeGetIsPlayerBirthsign                = 0x2000574;
        const int opcodeGetPCExpelled                       = 0x2000575;
        const int opcodeGetPCFactionAttack                  = 0x2000576;
        const int opcodeGetPCFactionMurder                  = 0x2000577;
        const int opcodeGetPCFactionSteal                   = 0x2000578;
        const int opcodeGetPCFactionSubmitAuthority         = 0x2000579;
        const int opcodeGetPCFame                           = 0x200057a;
        const int opcodeGetPCInFaction                      = 0x200057b;
        const int opcodeGetPCInfamy                         = 0x200057c;
        const int opcodeGetPCIsClass                        = 0x200057d;
        const int opcodeGetPCIsRace                         = 0x200057e;
        const int opcodeGetPCIsSex                          = 0x200057f;
        const int opcodeGetPCMiscStat                       = 0x2000580;
        const int opcodeGetPCSleepHours                     = 0x2000581;
        const int opcodeGetPlayerControlsDisabled           = 0x2000582;
        const int opcodeGetPlayerHasLastRiddenHorse         = 0x2000583;
        const int opcodeGetPlayerInSEWorld                  = 0x2000584;
        const int opcodeHasVampireFed                       = 0x2000585;
        const int opcodeHasVampireFedExplicit               = 0x2000586;
        const int opcodeIsPCAMurderer                       = 0x2000587;
        const int opcodeIsPCSleeping                        = 0x2000588;
        const int opcodeIsPlayerInJail                      = 0x2000589;
        const int opcodeIsPlayerMovingIntoNewSpace          = 0x200058a;
        const int opcodeIsPlayersLastRiddenHorse            = 0x200058b;
        const int opcodeIsPlayersLastRiddenHorseExplicit    = 0x200058c;
        const int opcodeIsTimePassing                       = 0x200058d;
        const int opcodeIsTurnArrest                        = 0x200058e;
        const int opcodeModAmountSoldStolen                 = 0x200058f;
        const int opcodeModPCAttribute                      = 0x2000590;
        const int opcodeModPCFame                           = 0x2000591;
        const int opcodeModPCInfamy                         = 0x2000592;
        const int opcodeModPCMiscStat                       = 0x2000593;
        const int opcodeModPCSkill                          = 0x2000594;
        const int opcodeSetInChargen                        = 0x2000595;
        const int opcodeSetPCExpelled                       = 0x2000596;
        const int opcodeSetPCFactionAttack                  = 0x2000597;
        const int opcodeSetPCFactionMurder                  = 0x2000598;
        const int opcodeSetPCFactionSteal                   = 0x2000599;
        const int opcodeSetPCFactionSubmitAuthority         = 0x200059a;
        const int opcodeSetPCFame                           = 0x200059b;
        const int opcodeSetPCInfamy                         = 0x200059c;
        const int opcodeSetPCSleepHours                     = 0x200059d;
        const int opcodeSetPlayerInSEWorld                  = 0x200059e;
        const int opcodeShowBirthSignMenu                   = 0x200059f;
        const int opcodeShowClassMenu                       = 0x20005a0;
        const int opcodeShowEnchantment                     = 0x20005a1;
        const int opcodeShowRaceMenu                        = 0x20005a2;
        const int opcodeShowSpellMaking                     = 0x20005a3;
        const int opcodeTriggerHitShader                    = 0x0020086; //
        const int opcodeWakeUpPC                            = 0x0020087; //
    }

    namespace Tes4Quest
    {
        const int opcodeCompleteQuest                       = 0x20005a4;
        const int opcodeGetQuestRunning                     = 0x20005a5;
        const int opcodeGetQuestVariable                    = 0x20005a6;
        const int opcodeGetStage                            = 0x20005a7;
        const int opcodeGetStageDone                        = 0x20005a8;
        const int opcodeSetStage                            = 0x20005a9;
        const int opcodeStartQuest                          = 0x20005aa;
        const int opcodeStopQuest                           = 0x20005ab;
    }

    namespace Tes4Statistics
    {
        const int opcodeGetArmorRating                      = 0x20005ac;
        const int opcodeGetArmorRatingExplicit              = 0x20005ad;
        const int opcodeGetArmorRatingUpperBody             = 0x20005ae;
        const int opcodeGetArmorRatingUpperBodyExplicit     = 0x20005af;
        const int opcodeGetBarterGold                       = 0x20005b0;
        const int opcodeGetBarterGoldExplicit               = 0x20005b1;
        const int opcodeGetFatiguePercentage                = 0x20005b2;
        const int opcodeGetGetFatiguePercentagelicit        = 0x20005b3;
        const int opcodeGetGold                             = 0x20005b4;
        const int opcodeGetGoldExplicit                     = 0x20005b5;
        const int opcodeGetInvestmentGold                   = 0x20005b6;
        const int opcodeGetInvestmentGoldExplicit           = 0x20005b7;
        const int opcodeGetLevel                            = 0x20005b8;
        const int opcodeGetLevelExplicit                    = 0x20005b9;
        const int opcodeGetScale                            = 0x20005ba;
        const int opcodeGetScaleExplicit                    = 0x20005bb;
        const int opcodeGetWalkSpeed                        = 0x20005bc;
        const int opcodeWalkSpeedExplicit                   = 0x20005bd;
        const int opcodeModBarterGold                       = 0x20005be;
        const int opcodeModBarterGoldExplicit               = 0x20005bf;
        const int opcodeModScale                            = 0x20005c0;
        const int opcodeModScaleExplicit                    = 0x20005c1;
        const int opcodeSetBarterGold                       = 0x20005c2;
        const int opcodeSetBarterGoldExplicit               = 0x20005c3;
        const int opcodeSetInvestmentGold                   = 0x20005c4;
        const int opcodeSetInvestmentGoldExplicit           = 0x20005c5;
        const int opcodeSetItemValue                        = 0x20005c6;
        const int opcodeSetItemValueExplicit                = 0x20005c7;
        const int opcodeSetScale                            = 0x20005c8;
        const int opcodeSetScaleExplicit                    = 0x20005c9;
    }

    namespace Tes4Weather
    {
        const int opcodeForceWeather                        = 0x0020088; //
        const int opcodeGetCurrentWeatherPercent            = 0x20005ca;
        const int opcodeGetIsCurrentWeather                 = 0x20005cb;
        const int opcodeGetWindSpeed                        = 0x20005cc;
        const int opcodeIsCloudy                            = 0x20005cd;
        const int opcodeIsPleasant                          = 0x20005ce;
        const int opcodeIsRaining                           = 0x20005cf;
        const int opcodeIsSnowing                           = 0x20005d0;
        const int opcodeReleaseWeatherOverride              = 0x20005d1;
        const int opcodeSetWeather                          = 0x0020089; //
    }

    namespace Tes4Misc
    {
        const int opcodeAddAchievement                      = 0x20005d2;
        const int opcodeAutosave                            = 0x20005d3;
        const int opcodeCloseCurrentOblivionGate            = 0x002008a; //
        const int opcodeCloseOblivionGate                   = 0x002008b; //
        const int opcodeCloseOblivionGateExplicit           = 0x002008c; //
        const int opcodeDisable                             = 0x20005d4;
        const int opcodeDisableExplicit                     = 0x20005d5;
        const int opcodeDisableLinkedPathPoints             = 0x20005d6;
        const int opcodeEnable                              = 0x20005d7;
        const int opcodeEnableExplicit                      = 0x20005d8;
        const int opcodeEnableFastTravel                    = 0x20005d9;
        const int opcodeEnableLinkedPathPoints              = 0x20005da;
        const int opcodeForceCloseOblivionGate              = 0x002008d; //
        const int opcodeEssentialDeathReload                = 0x20005db;
        const int opcodeGetActionRef                        = 0x20005dc;
        const int opcodeGetActionRefExplicit                = 0x20005dd;
        const int opcodeGetButtonPressed                    = 0x20005de;
        const int opcodeGetDestroyed                        = 0x20005df;
        const int opcodeGetDestroyedExplicit                = 0x20005e0;
        const int opcodeGetDoorDefaultOpen                  = 0x20005e1;
        const int opcodeGetDoorDefaultOpenExplicit          = 0x20005e2;
        const int opcodeGetGameSetting                      = 0x20005e3;
        const int opcodeGetGlobalValue                      = 0x20005e4;
        const int opcodeGetIdleDoneOnce                     = 0x20005e5;
        const int opcodeGetLocked                           = 0x20005e6;
        const int opcodeGetLockedExplicit                   = 0x20005e7;
        const int opcodeGetLockLevel                        = 0x20005e8;
        const int opcodeGetLockLevelExplicit                = 0x20005e9;
        const int opcodeGetOpenState                        = 0x20005ea;
        const int opcodeGetOpenStateExplicit                = 0x20005eb;
        const int opcodeGetPersuasionNumber                 = 0x20005ec;
        const int opcodeGetRandomPercent                    = 0x20005ed;
        const int opcodeGetScriptVariable                   = 0x20005ee;
        const int opcodeGetTotalPersuasionNumber            = 0x20005ef;
        const int opcodeIsActionRef                         = 0x20005f0;
        const int opcodeIsActionRefExplicit                 = 0x20005f1;
        const int opcodeIsXBox                              = 0x20005f2;
        const int opcodeLock                                = 0x002008e; //
        const int opcodeLockExplicit                        = 0x002008f; //
        const int opcodePlaceAtMe                           = 0x0020090; //
        const int opcodePlaceAtMeExplicit                   = 0x0020091; //
        const int opcodePlayBink                            = 0x0020092; //
        const int opcodePlaySound                           = 0x20005f3;
        const int opcodePlaySoundExplicit                   = 0x20005f4; // unexpected
        const int opcodePlaySound3D                         = 0x20005f5;
        const int opcodePurgeCellBuffers                    = 0x20005f6;
        const int opcodeReset3DState                        = 0x20005f7;
        const int opcodeResetFallDamageTimer                = 0x20005f8;
        const int opcodeResetFallDamageTimerExplicit        = 0x20005f9;
        const int opcodeResetInterior                       = 0x20005fa;
        const int opcodeSetActorFullName                    = 0x20005fb;
        const int opcodeSetActorFullNameExplicit            = 0x20005fc;
        const int opcodeSetCellFullName                     = 0x20005fd;
        const int opcodeSetDestroyed                        = 0x20005fe;
        const int opcodeSetDestroyedExplicit                = 0x20005ff;
        const int opcodeSetDoorDefaultOpen                  = 0x2020600;
        const int opcodeSetDoorDefaultOpenExplicit          = 0x2020601;
        const int opcodeSetOpenState                        = 0x2000602;
        const int opcodeSetOpenStateExplicit                = 0x2000603;
        const int opcodeSetSize                             = 0x2000604;
        const int opcodeSetSizeExplicit                     = 0x2000605;
        const int opcodeShowMap                             = 0x0020093; //
        const int opcodeStreamMusic                         = 0x2000606;
        const int opcodeTrapUpdate                          = 0x0020094; //
        const int opcodeUnLock                              = 0x0020095; //
        const int opcodeUnLockExplicit                      = 0x0020096; //
        const int opcodeWhichServiceMenu                    = 0x2000607;
        const int opcodeGetContainer                        = 0x2000608;
        const int opcodeGetContainerExplicit                = 0x2000609;
        const int opcodeGetPackageTarget                    = 0x200060a;
        const int opcodeGetPackageTargetExplicit            = 0x200060b;
        const int opcodeGetParentRef                        = 0x200060c;
        const int opcodeGetSelf                             = 0x200060d;
        const int opcodeGetCurrentTime                      = 0x200060e;
        const int opcodeGetDayOfWeek                        = 0x200060f;
        const int opcodeGetSecondsPassed                    = 0x2000610;
    }
}

#endif
