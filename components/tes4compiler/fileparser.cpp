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
      //mEventParser (errorHandler, context, mLocals),
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
        //if (mState == NameState)
        if (mState == StartState)
        {
            mName = name;
            mState = BeginState; // followed by either variable declarations or "begin" keyword
            mScriptParser.reset(); // FIXME: not the most obvious place to clear locals
            //mEventParser.reset();

            return true;
        }
#if 0
        // FIXME: untested
        if (mState == BlockTypeState)
        {
            std::cout << "file parser: event param " << loc.mLiteral << std::endl; // FIXME: temp testing
            // continue to Parser::parseName() below
        }
#endif
        return Parser::parseName (name, loc, scanner);
    }

    bool FileParser::parseKeyword (int keyword, const Compiler::TokenLoc& loc, Scanner& scanner)
    {
        // in TES4 script name comes after the keyword 'scn' or 'scriptname'
        if (mState == StartState && (keyword == Scanner::K_scn || keyword == Scanner::K_scriptname))
        {
            //mState = NameState; // TODO: see if possible to eliminate NameState

            return true;
        }

        // local variable declaration
        if (mState == BeginState && (keyword == Scanner::K_short || keyword == Scanner::K_long
                || keyword == Scanner::K_float || keyword == Scanner::K_ref))
        {
            //LineParser& lineParser = mEventParser.getLineParser();
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

            Compiler::Literals& mLiterals = mScriptParser.getLiterals();

            // below copied from ExprParser
            if (const Compiler::Extensions *extensions = getContext().getExtensions())
            {
                start();

                char returnType;
                std::string argumentType;
                bool hasExplicit = false;

                std::vector<Interpreter::Type_Code> code;

                if (extensions->isFunction (keyword, returnType, argumentType, hasExplicit))
                {
                    std::cout << "event: " << mBlockType << std::endl; // FIXME: temp testing

                    bool optional = false;
                    int optionalCount = 0;

                    ExprParser parser (getErrorHandler(), getContext(), mLocals, mLiterals, true);
                    StringParser stringParser (getErrorHandler(), getContext(), mLiterals);

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


                    if (optionalCount > 0)
                    {
                        extensions->generateFunctionCode(keyword, code, mLiterals, "", optionalCount);
                        //mOperands.push_back (returnType);
                    }

                    mScriptParser.reset(true/*keepLocals*/);
                    scanner.scan(mScriptParser);

                    if (getErrorHandler().isGood())
                    {
                        std::vector<Interpreter::Type_Code> empty;
                        std::pair<std::map<std::string, std::vector<Interpreter::Type_Code> >::iterator, bool> res
                            = mCodeBlocks.insert(std::make_pair(mBlockType, empty));

#if 0
                        typedef std::vector<Interpreter::Type_Code> Codes;
                        typedef std::vector<std::pair<Codes, Codes> > IfCodes;
                        Codes mCode; // resulting code
                        Codes mCodeBlock;
                        IfCodes mIfCode;

                        std::pair<Codes, Codes> entry;

                        // if condition code
                        //mExprParser.append(entry.first);
                        std::copy(code.begin(), code.end(), std::back_inserter(entry.first));

                        // if body code
                        //std::copy(mCodeBlock.begin(), mCodeBlock.end(), std::back_inserter(entry.second));
                        const std::vector<Interpreter::Type_Code>& output = mScriptParser.getCode();
                        std::copy(output.begin(), output.end(), std::back_inserter(entry.second));

                        mIfCode.push_back(entry);

                        mCodeBlock.clear();

                        {
                            Codes codes;

                            // FIXME: no need for a loop we only have one entry
                            for (IfCodes::reverse_iterator iter(mIfCode.rbegin()); iter != mIfCode.rend(); ++iter)
                            {
                                Codes block;

                                if (iter != mIfCode.rbegin())
                                    Generator::jump(iter->second, codes.size() + 1);

                                if (!iter->first.empty())
                                {
                                    // if or elseif
                                    std::copy(iter->first.begin(), iter->first.end(), std::back_inserter(block));
                                    Generator::jumpOnZero(block, iter->second.size() + 1);
                                }

                                std::copy(iter->second.begin(), iter->second.end(), std::back_inserter(block));

                                std::swap(codes, block);

                                std::copy(block.begin(), block.end(), std::back_inserter(codes));
                            }

                            std::copy(codes.begin(), codes.end(), std::back_inserter(mCode));

                            mIfCode.clear();
                        }
#else
                        const std::vector<Interpreter::Type_Code>& output = mScriptParser.getCode();
                        if (optionalCount > 0)
                            Generator::jumpOnZero(code, output.size() + 1);

                        std::copy(output.begin(), output.end(), std::back_inserter(code));

                        res.first->second.push_back(static_cast<Interpreter::Type_Code> (code.size()));

                        assert(mLiterals.getIntegerSize() % 4 == 0);
                        res.first->second.push_back(static_cast<Interpreter::Type_Code> (mLiterals.getIntegerSize() / 4));

                        assert(mLiterals.getFloatSize() % 4 == 0);
                        res.first->second.push_back(static_cast<Interpreter::Type_Code> (mLiterals.getFloatSize() / 4));

                        assert(mLiterals.getStringSize() % 4 == 0);
                        res.first->second.push_back(static_cast<Interpreter::Type_Code> (mLiterals.getStringSize() / 4));

                        // code
                        std::copy(code.begin(), code.end(), std::back_inserter(res.first->second));

                        // literals
                        mLiterals.append(res.first->second);


#endif

                        mState = BeginState; // continue in case there are other blocks

                    }
                }
            }
            // end of copy




            // FIXME: these functions/instructions can have arguments (hence no state change)
            // maybe generate a function and pass any parameters to it?
            // may also need to generate an if/else check
#if 0 // experiment
            // parse the script body
            mEventParser.reset(true/*keepLocals*/);

            scanner.putbackKeyword(keyword, loc); // FIXME: not the best place?
            scanner.scan (mEventParser);

            // since ScriptParser::mEnd is true the code scanning should be completed
            if (getErrorHandler().isGood())
            {
                std::vector<Interpreter::Type_Code> code;
                std::pair<std::map<std::string, std::vector<Interpreter::Type_Code> >::iterator, bool> res
                    = mCodeBlocks.insert(std::make_pair(mBlockType, code));
                mEventParser.getCode(res.first->second);
            }

            mState = BeginState; // continue in case there are other blocks
#endif

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
#if 0
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
#endif
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
        //mEventParser.reset();
        mScriptParser.reset();
        Parser::reset();
    }
}
