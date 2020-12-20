#include "scriptmanagerimp.hpp"

#include <cassert>
#include <iostream>
#include <sstream>
#include <exception>
#include <algorithm>
#include <iomanip> // FIXME

#include <extern/esm4/formid.hpp>
#include <extern/esm4/scpt.hpp>

#include <components/esm/loadscpt.hpp>

#include <components/misc/stringops.hpp>

#include <components/compiler/scanner.hpp>
#include <components/compiler/context.hpp>
#include <components/tes4compiler/scanner.hpp>
#include <components/compiler/exception.hpp>
#include <components/compiler/quickfileparser.hpp>
#include <components/tes4compiler/quickfileparser.hpp>

#include "../mwworld/esmstore.hpp"

#include "extensions.hpp"

namespace MWScript
{
    ScriptManager::ScriptManager (const MWWorld::ESMStore& store, bool verbose,
        Compiler::Context& compilerContext, Compiler::Context& tes4CompilerContext, int warningsMode,
        const std::vector<std::string>& scriptBlacklist)
    : mErrorHandler (std::cerr), mStore (store), mVerbose (verbose),
      mCompilerContext (compilerContext), mParser (mErrorHandler, mCompilerContext),
      mTes4CompilerContext(tes4CompilerContext), mTes4Parser (mErrorHandler, mTes4CompilerContext),
      mOpcodesInstalled (false), mGlobalScripts (store)
    {
        mErrorHandler.setWarningsMode (warningsMode);

        mScriptBlacklist.resize (scriptBlacklist.size());

        std::transform (scriptBlacklist.begin(), scriptBlacklist.end(),
            mScriptBlacklist.begin(), Misc::StringUtils::lowerCase);
        std::sort (mScriptBlacklist.begin(), mScriptBlacklist.end());
    }

    bool ScriptManager::compile (const std::string& name)
    {
        mParser.reset();
        mErrorHandler.reset();

        if (const ESM::Script *script = mStore.get<ESM::Script>().find (name))
        {
            if (mVerbose)
                std::cout << "compiling script: " << name << std::endl;

            bool Success = true;
            try
            {
                std::istringstream input (script->mScriptText);

                Compiler::Scanner scanner (mErrorHandler, input, mCompilerContext.getExtensions());

                scanner.scan (mParser);

                if (!mErrorHandler.isGood())
                    Success = false;
            }
            catch (const Compiler::SourceException&)
            {
                // error has already been reported via error handler
                Success = false;
            }
            catch (const std::exception& error)
            {
                std::cerr << "An exception has been thrown: " << error.what() << std::endl;
                Success = false;
            }

            if (!Success)
            {
                std::cerr
                    << "compiling failed: " << name << std::endl;
                if (mVerbose)
                    std::cerr << script->mScriptText << std::endl << std::endl;
            }

            if (Success)
            {
                std::vector<Interpreter::Type_Code> code;
                mParser.getCode (code);
                mScripts.insert (std::make_pair (name, std::make_pair (code, mParser.getLocals())));

                return true;
            }
        }

        return false;
    }

    bool ScriptManager::compileForeign (const std::string& name)
    {
        mTes4Parser.reset();
        mErrorHandler.reset();

        if (const ESM4::Script *script = mStore.getForeign<ESM4::Script>().search (ESM4::stringToFormId(name)))
        {
            if (mVerbose)
                std::cout << "compiling script: " << name << std::endl;

            bool Success = true;
            try
            {
                std::istringstream input (script->mScript.scriptSource);

                Tes4Compiler::Scanner scanner (mErrorHandler, input, mTes4CompilerContext.getExtensions());

                scanner.scan (mTes4Parser);

                if (!mErrorHandler.isGood())
                    Success = false;
            }
            catch (const Compiler::SourceException&)
            {
                // error has already been reported via error handler
                Success = false;
            }
            catch (const std::exception& error)
            {
                std::cerr << "An exception has been thrown: " << error.what() << std::endl;
                Success = false;
            }

            if (!Success)
            {
                std::cerr
                    << "compiling a foreign script failed: " << name << std::endl;
                if (mVerbose)
                    std::cerr << script->mScript.scriptSource << std::endl << std::endl;
            }

            if (Success)
            {
                ScriptCollection codes;
                mTes4Parser.getCode (codes);

                std::cout << "compiled " << name << " " << mTes4Parser.getName() << std::endl; // FIXME: temp testing

                // FIXME: so much copying - maybe use unique_ptr ?
                mForeignScripts.insert (std::make_pair (name, codes));

                return true;
            }
        }

        return false;
    }

    void ScriptManager::run (const std::string& name, Interpreter::Context& interpreterContext,
                              const std::string& blockType)
    {
        if (ESM4::isFormId(name))
            return runForeign(name, interpreterContext, blockType);

        // compile script
        ScriptCollection::iterator iter = mScripts.find (name);

        if (iter==mScripts.end())
        {
            if (!compile (name))
            {
                // failed -> ignore script from now on.
                std::vector<Interpreter::Type_Code> empty;
                mScripts.insert (std::make_pair (name, std::make_pair (empty, Compiler::Locals())));
                return;
            }

            iter = mScripts.find (name);
            assert (iter!=mScripts.end());
        }

        // execute script
        if (!iter->second.first.empty())
            try
            {
                if (!mOpcodesInstalled)
                {
                    installOpcodes (mInterpreter);
                    mOpcodesInstalled = true;
                }

                mInterpreter.run (&iter->second.first[0], iter->second.first.size(), interpreterContext);
            }
            catch (const std::exception& e)
            {
                std::cerr << "Execution of script " << name << " failed:" << std::endl;
                std::cerr << e.what() << std::endl;

                iter->second.first.clear(); // don't execute again.
            }
    }

    // NOTE: Variables in interpreter context and compiler context
    //
    // The local variables in interpreter context come from the run-time data of the object to which
    // the script is attached.  See World::activate() as an example.
    //
    // Those local variables are created during the loading of the cell in which the object
    // references are located.  If the run-time data were saved then the local variables'
    // contents are restored when the saved game is loaded.
    void ScriptManager::runForeign (const std::string& name, Interpreter::Context& interpreterContext,
                              const std::string& blockType)
    {
        // compile script
        std::map<std::string, ScriptCollection>::iterator iter = mForeignScripts.find (name);

        if (iter == mForeignScripts.end())
        {
            if (!compileForeign(name))
            {
                // failed -> ignore script from now on.
                std::vector<Interpreter::Type_Code> empty;
                ScriptCollection emptyMap;
                emptyMap.insert(std::make_pair(std::string(), std::make_pair(empty, Compiler::Locals())));
                mForeignScripts.insert (std::make_pair (name, emptyMap));
                return;
            }

            iter = mForeignScripts.find (name);
            assert (iter != mForeignScripts.end());
        }

        //std::cout << "run foreign script: " << name << std::endl; // FIXME: temp testing

        // execute script
        std::string blockName = "gamemode";
        if (blockType != std::string())
            blockName = blockType;

        ScriptCollection::iterator iter2 = iter->second.find(blockName);
        if (iter2 != iter->second.end() && !iter2->second.first.empty())
            try
            {
                if (!mOpcodesInstalled)
                {
                    installOpcodes (mInterpreter);
                    mOpcodesInstalled = true;
                }

#if 0 // FIXME: temp testing
                if (name == "00083192" && blockName == "gamemode")
                for (unsigned int i = 0; i < iter2->second.first.size(); ++i)
                    std::cout << "0x" << std::setfill('0') << std::setw(8) << std::hex << iter2->second.first[i] << std::endl;
#endif
                mInterpreter.run (&iter2->second.first[0], iter2->second.first.size(), interpreterContext);
            }
            catch (const std::exception& e)
            {
                std::cerr << "Execution of foreign script " << name << " failed:" << std::endl;
                std::cerr << e.what() << std::endl;

                iter2->second.first.clear(); // don't execute again.
            }
    }

    std::pair<int, int> ScriptManager::compileAll()
    {
        int count = 0;
        int success = 0;

        const MWWorld::Store<ESM::Script>& scripts = mStore.get<ESM::Script>();

        for (MWWorld::Store<ESM::Script>::iterator iter = scripts.begin();
            iter != scripts.end(); ++iter)
            if (!std::binary_search (mScriptBlacklist.begin(), mScriptBlacklist.end(),
                Misc::StringUtils::lowerCase (iter->mId)))
            {
                ++count;

                if (compile (iter->mId))
                    ++success;
            }

        return std::make_pair (count, success);
    }

    const Compiler::Locals& ScriptManager::getLocals (const std::string& name)
    {
        // FIXME: not very efficient, probably best to add another method in MWBase::ScriptManager
        if (ESM4::isFormId(name))
            return getForeignLocals(name);

        std::string name2 = Misc::StringUtils::lowerCase (name);

        {
            ScriptCollection::iterator iter = mScripts.find (name2);

            if (iter!=mScripts.end())
                return iter->second.second;
        }

        {
            std::map<std::string, Compiler::Locals>::iterator iter = mOtherLocals.find (name2);

            if (iter!=mOtherLocals.end())
                return iter->second;
        }

        if (const ESM::Script *script = mStore.get<ESM::Script>().find (name2))
        {
            if (mVerbose)
                std::cout
                    << "scanning script for local variable declarations: " << name2
                    << std::endl;

            Compiler::Locals locals;

            std::istringstream stream (script->mScriptText);
            Compiler::QuickFileParser parser (mErrorHandler, mCompilerContext, locals);
            Compiler::Scanner scanner (mErrorHandler, stream, mCompilerContext.getExtensions());
            scanner.scan (parser);

            std::map<std::string, Compiler::Locals>::iterator iter =
                mOtherLocals.insert (std::make_pair (name2, locals)).first;

            return iter->second;
        }

        throw std::logic_error ("script " + name + " does not exist");
    }

    // probably used for loading save files
    const Compiler::Locals& ScriptManager::getForeignLocals (const std::string& name)
    {
        ScriptCollection::iterator iter = mScripts.find (name);

        if (iter != mScripts.end())
            return iter->second.second;

        std::map<std::string, Compiler::Locals>::iterator iterOther = mOtherLocals.find (name);

        if (iterOther != mOtherLocals.end())
            return iterOther->second;

        if (const ESM4::Script *script = mStore.getForeign<ESM4::Script>().search (ESM4::stringToFormId(name)))
        {
            if (mVerbose)
                std::cout
                    << "scanning script for local variable declarations: " << name
                    << std::endl;

            Compiler::Locals locals;

            std::istringstream stream (script->mScript.scriptSource);
            Tes4Compiler::QuickFileParser parser (mErrorHandler, mTes4CompilerContext, locals);
            Tes4Compiler::Scanner scanner (mErrorHandler, stream, mTes4CompilerContext.getExtensions());
            scanner.scan (parser);

            std::map<std::string, Compiler::Locals>::iterator iterOther2 =
                mOtherLocals.insert (std::make_pair (name, locals)).first;

            return iterOther2->second;
        }

        throw std::logic_error ("script " + name + " does not exist");
    }

    GlobalScripts& ScriptManager::getGlobalScripts()
    {
        return mGlobalScripts;
    }
}
