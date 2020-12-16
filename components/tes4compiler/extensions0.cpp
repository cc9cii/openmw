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
            extensions.registerFunction ("onactivate", 'l', "/c", opcodeOnActivateRef, opcodeOnActivateRefExplicit);
        }
    }

    namespace Tes4
    {
        void registerExtensions (Compiler::Extensions& extensions)
        {
            extensions.registerFunction ("isactionref", 'l', "c", opcodeIsActionRef, opcodeIsActionRefExplicit);

            extensions.registerFunction ("getstage", 'l', "c", opcodeGetStage, opcodeGetStageExplicit);

            extensions.registerFunction ("getlocked", 'l', "", opcodeGetLocked, -1);

            extensions.registerInstruction ("activate", "/cl", opcodeActivateRef);
        }
    }

}
