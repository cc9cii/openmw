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
            extensions.registerInstruction ("onactivate", "/c", opcodeOnActivate);
            extensions.registerInstruction ("gamemode", "", opcodeGameMode);
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
        }
    }

}
