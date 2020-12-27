#ifndef GAME_SCRIPT_SCRIPTMANAGER_H
#define GAME_SCRIPT_SCRIPTMANAGER_H

#include <map>
#include <string>

#include <components/compiler/streamerrorhandler.hpp>
#include <components/compiler/fileparser.hpp>
#include <components/tes4compiler/fileparser.hpp>

#include <components/interpreter/interpreter.hpp>
#include <components/interpreter/types.hpp>

#include "../mwbase/scriptmanager.hpp"

#include "globalscripts.hpp"

namespace MWWorld
{
    class ESMStore;
}

namespace Compiler
{
    class Context;
}

namespace Tes4Compiler
{
    class Context;
}

namespace Interpreter
{
    class Context;
    class Interpreter;
}

namespace MWScript
{
    class ScriptManager : public MWBase::ScriptManager
    {
            Compiler::StreamErrorHandler mErrorHandler;
            const MWWorld::ESMStore& mStore;
            bool mVerbose;
            Compiler::Context& mCompilerContext;
            Compiler::FileParser mParser;
            Compiler::Context& mTes4CompilerContext;
            Tes4Compiler::FileParser mTes4Parser;
            Interpreter::Interpreter mInterpreter;
            bool mOpcodesInstalled;

            typedef std::pair<std::vector<Interpreter::Type_Code>, Compiler::Locals> CompiledScript;
            typedef std::map<std::string, CompiledScript> ScriptCollection;

            ScriptCollection mScripts;
            std::map<std::string, ScriptCollection> mForeignScripts;
            GlobalScripts mGlobalScripts;
            std::map<std::string, Compiler::Locals> mOtherLocals;
            std::vector<std::string> mScriptBlacklist;

        public:

            ScriptManager (const MWWorld::ESMStore& store, bool verbose,
                Compiler::Context& compilerContext, Compiler::Context& tes4CompilerContext, int warningsMode,
                const std::vector<std::string>& scriptBlacklist);

            virtual bool run (const std::string& name, Interpreter::Context& interpreterContext,
                              const std::string& blockType = std::string());
            ///< Run the script with the given name (compile first, if not compiled yet)

            bool runForeign (const std::string& name, Interpreter::Context& interpreterContext,
                              const std::string& blockType = std::string());

            virtual bool compile (const std::string& name);
            bool compileForeign (const std::string& name);
            ///< Compile script with the given namen
            /// \return Success?

            virtual std::pair<int, int> compileAll();
            ///< Compile all scripts
            /// \return count, success

            virtual const Compiler::Locals& getLocals (const std::string& name);
            ///< Return locals for script \a name.

            const Compiler::Locals& getForeignLocals (const std::string& name);

            virtual GlobalScripts& getGlobalScripts();
    };
}

#endif
