#include "fileparser.hpp"

#include <iostream> // FIXME

#include "../compiler/tokenloc.hpp"
#include "../compiler/errorhandler.hpp"
#include "scanner.hpp"
#include "output.hpp"

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

    const std::string& FileParser::getBlockType() const
    {
        // FIXME: for testing use the last one
        return mCodeBlocks.rbegin()->first;
    }

    void FileParser::getCode (std::vector<Interpreter::Type_Code>& code) const
    {
        // FIXME: for testing use the last one
        std::copy(mCodeBlocks.rbegin()->second.begin(), mCodeBlocks.rbegin()->second.end(), std::back_inserter(code));
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
            mBlockType = loc.mLiteral;

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
                mScriptParser.reset(); // NOTE: this clears previous code block!

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
