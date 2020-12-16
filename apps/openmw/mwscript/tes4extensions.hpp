#ifndef GAME_SCRIPT_TES4EXTENSIONS_H
#define GAME_SCRIPT_TES4EXTENSIONS_H

namespace Compiler
{
    class Extensions;
}

namespace Interpreter
{
    class Interpreter;
}

namespace MWScript
{
    /// \brief Tes4-related script functionality
    namespace Tes4
    {
        void installOpcodes (Interpreter::Interpreter& interpreter);
    }
}

#endif
