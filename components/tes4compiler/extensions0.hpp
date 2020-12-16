#ifndef TES4COMPILER_EXTENSIONS0_H
#define TES4COMPILER_EXTENSIONS0_H

namespace Compiler
{
    class Extensions;
}

namespace Tes4Compiler
{
    void registerExtensions (Compiler::Extensions& extensions, bool consoleOnly = false);

    namespace Event
    {
        void registerExtensions (Compiler::Extensions& extensions);
    }

    namespace Tes4
    {
        void registerExtensions (Compiler::Extensions& extensions);
    }
}

#endif
