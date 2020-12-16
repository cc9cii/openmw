#include "fileparser.hpp"

#include <iostream>

#include "../compiler/tokenloc.hpp"
#include "scanner.hpp"
#include "output.hpp"

namespace Tes4Compiler
{
    FileParser::FileParser (Compiler::ErrorHandler& errorHandler, Compiler::Context& context)
    : Parser (errorHandler, context),
      mScriptParser (errorHandler, context, mLocals, true),
      //mDeclarationParser (errorHandler, context, mLocals),
      // FIXME: ugly hack to supply dummy literals and code to the LineParser
      mLineParser (errorHandler, context, mLocals, Output(mLocals).getLiterals(), Output(mLocals).getCode()),
      mState (StartState)
    {}

    std::string FileParser::getName() const
    {
        return mName;
    }

    void FileParser::getCode (std::vector<Interpreter::Type_Code>& code) const
    {
        mScriptParser.getCode (code);
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
            std::cout << "script name: " << mName << std::endl; // FIXME: temp testing
            mState = BeginState;

            return true;
        }

        if (mState == EndNameState) // TODO: does this occur for TES4 as well?
        {
            // optional repeated name after end statement
            if (mName!=name)
                reportWarning ("Names for script " + mName + " do not match", loc);

            mState = EndCompleteState;

            return false; // we are stopping here, because there might be more garbage on the end line,
                          // that we must ignore.
                          //
                          /// \todo allow this workaround to be disabled for newer scripts
        }

        if (mState == BeginCompleteState)
        {
            reportWarning ("Stray string (" + name + ") after begin statement", loc);
            return true;
        }

        return Parser::parseName (name, loc, scanner);
    }

    bool FileParser::parseKeyword (int keyword, const Compiler::TokenLoc& loc, Scanner& scanner)
    {
        // in TES4 script name comes after the keyword 'scn' or 'scriptname'
        if (mState == StartState && (keyword == Scanner::K_scn || keyword == Scanner::K_scriptname))
        {
            mState = NameState; // FIXME: do we need really a new state here?
            return true;
        }

        if (mState == BeginState && (keyword == Scanner::K_short || keyword == Scanner::K_long
                || keyword == Scanner::K_float || keyword == Scanner::K_ref))
        {
            //mDeclarationParser.reset();
            mLineParser.reset();
            scanner.putbackKeyword (keyword, loc);
            //scanner.scan (mDeclarationParser);
            scanner.scan (mLineParser);

            return true;
        }

        if (mState == BeginState && keyword == Scanner::K_begin)
        {
            // FIXME: how to deal with different block types?

            std::cout << "file parser keyword: begin " << std::endl; // FIXME: temp testing
            mState = BlockTypeState;

            return true;
        }

        if (mState == BlockTypeState)
        {
            // FIXME: these functions/instructions can have arguments
            std::cout << "file parser: event " << loc.mLiteral << std::endl; // FIXME: temp testing

            mState = BeginCompleteState;
            return true;
        }

// TODO: check if Oblivion also allows keywords to be used as script names
#if 0
        if (mState == NameState)
        {
            // keywords can be used as script names too. Thank you Morrowind for another
            // syntactic perversity :(
            mName = loc.mLiteral;
            mState = BeginCompleteState;
            return true;
        }
#endif
        if (mState == EndNameState)
        {
            // optional repeated name after end statement
            if (mName != loc.mLiteral)
                reportWarning ("Names for script " + mName + " do not match", loc);

            mState = EndCompleteState;
            return false; // we are stopping here, because there might be more garbage on the end line,
                          // that we must ignore.
                          //
                          /// \todo allow this workaround to be disabled for newer scripts
        }

        return Parser::parseKeyword (keyword, loc, scanner);
    }

    bool FileParser::parseSpecial (int code, const Compiler::TokenLoc& loc, Scanner& scanner)
    {
        if (code==Scanner::S_newline)
        {
            if (mState==BeginState)
            {
                // ignore empty lines
                return true;
            }

            if (mState == BeginCompleteState)
            {
                // parse the script body
                mScriptParser.reset();

                scanner.scan (mScriptParser);

                mState = EndNameState;
                return true;
            }

            if (mState==EndCompleteState || mState==EndNameState)
            {
                // we are done here -> ignore the rest of the script
                return false;
            }
        }

        return Parser::parseSpecial (code, loc, scanner);
    }

    void FileParser::parseEOF (Scanner& scanner)
    {
        if (mState!=EndNameState && mState!=EndCompleteState)
            Parser::parseEOF (scanner);
    }

    void FileParser::reset()
    {
        mState = StartState;
        mName.clear();
        mScriptParser.reset();
        Parser::reset();
    }
}
