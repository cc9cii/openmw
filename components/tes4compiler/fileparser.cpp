#include "fileparser.hpp"

#include <iostream> // FIXME

#include <components/misc/stringops.hpp>

#include "../compiler/tokenloc.hpp"
#include "../compiler/errorhandler.hpp"
#include "scanner.hpp"
#include "output.hpp"
#include "lineparser.hpp"

namespace Tes4Compiler
{
    FileParser::FileParser (Compiler::ErrorHandler& errorHandler, Compiler::Context& context)
    : Parser (errorHandler, context),
      mScriptParser (errorHandler, context, mLocals, true/*end keyword marks end of script*/),
      mState (StartState), mName(""), mBlockType("")
    {}

    const std::string& FileParser::getName() const
    {
        return mName;
    }

    void FileParser::getCode (std::map<std::string,
                                       std::pair<std::vector<Interpreter::Type_Code>,
                                                 Compiler::Locals> >& codes) const
    {
        std::map <std::string, std::vector<Interpreter::Type_Code> >::const_iterator cit = mCodeBlocks.begin();
        for (; cit != mCodeBlocks.end(); ++cit)
        {
            std::vector<Interpreter::Type_Code> empty;
            std::pair<std::map<std::string,
                               std::pair<std::vector<Interpreter::Type_Code>,
                                         Compiler::Locals> >::iterator, bool> res
                = codes.insert(std::make_pair(cit->first, std::make_pair (empty, Compiler::Locals())));

            std::copy(cit->second.begin(), cit->second.end(), std::back_inserter(res.first->second.first));
        }
    }

    const Compiler::Locals& FileParser::getLocals() const
    {
        return mLocals;
    }

    bool FileParser::parseName (const std::string& name, const Compiler::TokenLoc& loc,
        Scanner& scanner)
    {
        if (mState == NameState)
        {
            mName = name;
            mState = BeginState; // followed by either variable declarations or "begin" keyword
            mScriptParser.reset(); // FIXME: not the most obvious place to clear locals

            return true;
        }

        // FIXME: untested
        if (mState == BlockTypeState)
        {
            std::cout << "file parser: event param " << loc.mLiteral << std::endl; // FIXME: temp testing
            // continue to Parser::parseName() below
        }

        return Parser::parseName (name, loc, scanner);
    }

    bool FileParser::parseKeyword (int keyword, const Compiler::TokenLoc& loc, Scanner& scanner)
    {
        // in TES4 script name comes after the keyword 'scn' or 'scriptname'
        if (mState == StartState && (keyword == Scanner::K_scn || keyword == Scanner::K_scriptname))
        {
            mState = NameState; // TODO: see if possible to eliminate NameState

            return true;
        }

        // local variable declaration
        if (mState == BeginState && (keyword == Scanner::K_short || keyword == Scanner::K_long
                || keyword == Scanner::K_float || keyword == Scanner::K_ref))
        {
            LineParser& lineParser = mScriptParser.getLineParser();
            lineParser.reset();
            scanner.putbackKeyword (keyword, loc);
            scanner.scan (lineParser);

            return true;
        }

        // begin blocktype
        if (mState == BeginState && keyword == Scanner::K_begin)
        {
            mState = BlockTypeState; // TODO: see if possible to eliminate BlockTypeState

            return true;
        }

        if (mState == BlockTypeState)
        {
            // NOTE: must be set to lower case because the caller will expect lower case
            mBlockType = Misc::StringUtils::lowerCase (loc.mLiteral);

            // these functions/instructions can have arguments (hence no state change)
            return true;
        }

        return Parser::parseKeyword (keyword, loc, scanner);
    }

    bool FileParser::parseSpecial (int code, const Compiler::TokenLoc& loc, Scanner& scanner)
    {
        if (code == Scanner::S_newline)
        {
            if (mState == BeginState)
            {
                // ignore empty lines
                return true;
            }

            if (mState == BlockTypeState || mState == BeginCompleteState)
            {
                // parse the script body
                mScriptParser.reset(true/*keepLocals*/);

                scanner.scan (mScriptParser);

                // since ScriptParser::mEnd is true the code scanning should be completed
                if (getErrorHandler().isGood())
                {
                    std::vector<Interpreter::Type_Code> code;
                    std::pair<std::map<std::string, std::vector<Interpreter::Type_Code> >::iterator, bool> res
                        = mCodeBlocks.insert(std::make_pair(mBlockType, code));
                    mScriptParser.getCode(res.first->second);
                }

                mState = BeginState; // continue in case there are other blocks

                return true;
            }
        }

        return Parser::parseSpecial (code, loc, scanner);
    }

    void FileParser::parseEOF (Scanner& scanner)
    {
        if (mState != BeginState)
            Parser::parseEOF (scanner);
    }

    void FileParser::reset()
    {
        mCodeBlocks.clear();
        mLocals.clear();
        mState = StartState;
        mBlockType.clear();
        mName.clear();
        mScriptParser.reset();
        Parser::reset();
    }
}
