#include "fileparser.hpp"

#include <iostream> // FIXME
#include <stack>
//#include <iterator>
//#include <algorithm>

#include <components/misc/stringops.hpp>

#include "../compiler/tokenloc.hpp"
#include "../compiler/errorhandler.hpp"
#include "scanner.hpp"
#include "output.hpp"
#include "lineparser.hpp"

#include "../compiler/context.hpp"
#include "../compiler/extensions.hpp"
#include "stringparser.hpp"
#include "generator.hpp"

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
        if (mState == StartState)
        {
            mName = name;
            mState = BeginState; // followed by either variable declarations or "begin" keyword
            mScriptParser.reset(); // FIXME: not the most obvious place to clear locals

            return true;
        }

        return Parser::parseName (name, loc, scanner);
    }

    bool FileParser::parseKeyword (int keyword, const Compiler::TokenLoc& loc, Scanner& scanner)
    {
        // in TES4 script name comes after the keyword 'scn' or 'scriptname'
        if (mState == StartState && (keyword == Scanner::K_scn || keyword == Scanner::K_scriptname))
        {
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
            mState = BlockTypeState;

            return true;
        }

        if (mState == BlockTypeState)
        {
            // NOTE: must be set to lower case because the caller will expect lower case
            mBlockType = Misc::StringUtils::lowerCase(loc.mLiteral);

            if (const Compiler::Extensions *extensions = getContext().getExtensions())
            {
                start();

                char returnType;
                std::string argumentType;
                bool hasExplicit = false;

                if (extensions->isFunction (keyword, returnType, argumentType, hasExplicit))
                {
                    Compiler::Literals& literals = mScriptParser.getLiterals();
                    const std::vector<Interpreter::Type_Code>& output = mScriptParser.getCode();
                    std::vector<Interpreter::Type_Code> code;

                    int optionalCount = parseArguments(argumentType, scanner, code, literals);

                    if (optionalCount > 0)
                    {
                        extensions->generateFunctionCode(keyword, code, literals, "", optionalCount);
                        //mOperands.push_back (returnType);
                    }

                    std::cout << "event: " << mBlockType << std::endl; // FIXME: temp testing

                    mScriptParser.reset(true/*keepLocals*/);
                    scanner.scan(mScriptParser);

                    if (optionalCount > 0)
                    {
                        Generator::jumpOnZero(code, (int) output.size() + 1); // keep compiler quiet
                    }

                    if (getErrorHandler().isGood())
                    {
                        getBlockCode(literals, output, code);
                    }
                }
            }

            mState = BeginState; // continue in case there are other blocks

            return true;
        }

        return Parser::parseKeyword (keyword, loc, scanner);
    }

    void FileParser::getBlockCode (Compiler::Literals& literals,
        const std::vector<Interpreter::Type_Code>& output, std::vector<Interpreter::Type_Code>& code)
    {
        std::vector<Interpreter::Type_Code> empty;
        std::pair<std::map<std::string, std::vector<Interpreter::Type_Code> >::iterator, bool> res
            = mCodeBlocks.insert(std::make_pair(mBlockType, empty));

        std::copy(output.begin(), output.end(), std::back_inserter(code));

        res.first->second.push_back(static_cast<Interpreter::Type_Code> (code.size()));

        assert(literals.getIntegerSize() % 4 == 0);
        res.first->second.push_back(static_cast<Interpreter::Type_Code> (literals.getIntegerSize() / 4));

        assert(literals.getFloatSize() % 4 == 0);
        res.first->second.push_back(static_cast<Interpreter::Type_Code> (literals.getFloatSize() / 4));

        assert(literals.getStringSize() % 4 == 0);
        res.first->second.push_back(static_cast<Interpreter::Type_Code> (literals.getStringSize() / 4));

        // code
        std::copy(code.begin(), code.end(), std::back_inserter(res.first->second));

        // literals
        literals.append(res.first->second);
    }

    int FileParser::parseArguments (const std::string& argumentType, Scanner& scanner,
        std::vector<Interpreter::Type_Code>& code, Compiler::Literals& literals)
    {
        bool optional = false;
        int optionalCount = 0;

        ExprParser parser (getErrorHandler(), getContext(), mLocals, literals, true);
        StringParser stringParser (getErrorHandler(), getContext(), literals);

        std::stack<std::vector<Interpreter::Type_Code> > stack;

        for (std::string::const_iterator iter (argumentType.begin()); iter!=argumentType.end(); ++iter)
        {
            if (*iter == '/')
            {
                optional = true;
            }
            else if (/* *iter=='S' || */ *iter=='c' /*|| *iter=='x'*/)
            {
                stringParser.reset();

                if (optional /*|| *iter=='x'*/)
                    stringParser.setOptional (true);

                if (*iter == 'c')
                    stringParser.smashCase();

                scanner.scan (stringParser);

                if (optional && stringParser.isEmpty())
                    break;

                if (*iter != 'x')
                {
                    std::vector<Interpreter::Type_Code> tmp;
                    stringParser.append (tmp);

                    stack.push (tmp);

                    if (optional)
                        ++optionalCount;
                }
            }
            else
            {
                parser.reset();

                if (optional)
                    parser.setOptional (true);

                scanner.scan (parser);

                if (optional && parser.isEmpty())
                    break;

                std::vector<Interpreter::Type_Code> tmp;

                char type = parser.append (tmp);

                if (type != *iter)
                    Generator::convert (tmp, type, *iter);

                stack.push (tmp);

                if (optional)
                    ++optionalCount;
            }
        }

        while (!stack.empty())
        {
            std::vector<Interpreter::Type_Code>& tmp = stack.top();

            std::copy (tmp.begin(), tmp.end(), std::back_inserter (code));

            stack.pop();
        }

        return optionalCount;
    }

    bool FileParser::parseSpecial (int code, const Compiler::TokenLoc& loc, Scanner& scanner)
    {
        if (code == Scanner::S_newline)
        {
            if (mState == BeginState)
            {
                return true; // ignore empty lines
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
