#include "extensions0.hpp"

#include "opcodes.hpp"
#include "../compiler/extensions.hpp"

namespace Tes4Compiler
{
    void registerExtensions (Compiler::Extensions& extensions, bool consoleOnly)
    {
        Event::registerExtensions (extensions);
        Tes4::registerExtensions (extensions);
    }

    namespace Event
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
            extensions.registerFunction ("onpackagechange",  'l', "",    opcodeOnPackageChange);
            extensions.registerFunction ("onpackagedone",    'l', "",    opcodeOnPackageDone);
            extensions.registerFunction ("onpackageend",     'l', "",    opcodeOnPackageEnd);
            extensions.registerFunction ("onpackagestart",   'l', "",    opcodeOnPackageStart);
            extensions.registerFunction ("onreset",          'l', "",    opcodeOnReset);
            extensions.registerFunction ("onsell",           'l', "/c",  opcodeOnSell);
            extensions.registerFunction ("onstartcombat",    'l', "/c",  opcodeOnStartCombat);
            extensions.registerFunction ("ontrigger",        'l', "/c",  opcodeOnTrigger);
            extensions.registerFunction ("ontriggeractor",   'l', "/c",  opcodeOnTriggerActor);
            extensions.registerFunction ("ontriggermob",     'l', "/c",  opcodeOnTriggerMob);
            extensions.registerFunction ("onunequip",        'l', "/c",  opcodeOnUnequip);
        }
    }

    namespace Tes4
    {
        void registerExtensions (Compiler::Extensions& extensions)
        {
            extensions.registerFunction ("isactionref", 'l', "c", opcodeIsActionRef, opcodeIsActionRefExplicit);

            extensions.registerFunction ("getstage", 'l', "c", opcodeGetStage, opcodeGetStageExplicit);

            // NOTE: opcodeGetLocked is in Tes4 namespace i.e. shouldn't clash with Misc::opcodeGetLocked
            extensions.registerFunction ("getlocked", 'l', "", opcodeGetLocked, -1);

            extensions.registerInstruction ("activate", "/cl", opcodeActivate, opcodeActivateExplicit);

            extensions.registerInstruction ("playgroup", "cl", opcodePlayGroup);

            extensions.registerFunction ("getself", 'l', "", opcodeGetSelf, -1);
            extensions.registerFunction ("this", 'l', "", opcodeGetSelf, -1);

            extensions.registerFunction ("getparentref", 'l', "", opcodeGetParentRef, -1);

            extensions.registerFunction ("isanimplaying", 'l', "", opcodeIsAnimPlaying, -1);

            extensions.registerInstruction ("disablelinkedpathpoints", "", opcodeDisableLinkedPathPoints, -1);

            extensions.registerInstruction ("setstage", "cl", opcodeSetStage, -1);

            extensions.registerInstruction ("disable", "", opcodeDisable, -1);
        }
    }

}
