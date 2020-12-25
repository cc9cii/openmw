#ifndef TES4COMPILER_EXTENSIONS0_H
#define TES4COMPILER_EXTENSIONS0_H

namespace Compiler
{
    class Extensions;
}

namespace Tes4Compiler
{
    void registerExtensions (Compiler::Extensions& extensions, bool consoleOnly = false);

    namespace Tes4Event
    {
        void registerExtensions (Compiler::Extensions& extensions);
    }

    namespace Tes4Actor
    {
        void registerExtensions (Compiler::Extensions& extensions);
    }

    namespace Tes4AI
    {
        void registerExtensions (Compiler::Extensions& extensions);
    }

    namespace Tes4Animation
    {
        void registerExtensions (Compiler::Extensions& extensions);
    }

    namespace Tes4Combat
    {
        void registerExtensions (Compiler::Extensions& extensions);
    }

    namespace Tes4Crime
    {
        void registerExtensions (Compiler::Extensions& extensions);
    }

    namespace Tes4Dialogue
    {
        void registerExtensions (Compiler::Extensions& extensions);
    }

    namespace Tes4Faction
    {
        void registerExtensions (Compiler::Extensions& extensions);
    }

    namespace Tes4Inventory
    {
        void registerExtensions (Compiler::Extensions& extensions);
    }

    namespace Tes4Magic
    {
        void registerExtensions (Compiler::Extensions& extensions);
    }

    namespace Tes4Movement
    {
        void registerExtensions (Compiler::Extensions& extensions);
    }

    namespace Tes4Player
    {
        void registerExtensions (Compiler::Extensions& extensions);
    }

    namespace Tes4Quest
    {
        void registerExtensions (Compiler::Extensions& extensions);
    }

    namespace Tes4Statistics
    {
        void registerExtensions (Compiler::Extensions& extensions);
    }

    namespace Tes4Weather
    {
        void registerExtensions (Compiler::Extensions& extensions);
    }

    namespace Tes4Misc
    {
        void registerExtensions (Compiler::Extensions& extensions);
    }
}

#endif
