#include "stringparser.hpp"

#include <algorithm>
#include <iterator>

#include <components/misc/stringops.hpp>

#include "scanner.hpp"
#include "generator.hpp"
#include "../compiler/locals.hpp"
#include "../compiler/context.hpp"
#include "../compiler/extensions.hpp"

namespace Tes4Compiler
{
    StringParser::StringParser (Compiler::ErrorHandler& errorHandler, const Compiler::Context& context, Compiler::Locals& locals, Compiler::Literals& literals)
    : Parser (errorHandler, context), mLocals (locals), mLiterals (literals), mState (StartState), mSmashCase (false)
    {

    }

    bool StringParser::parseName (const std::string& name, const Compiler::TokenLoc& loc,
        Scanner& scanner)
    {
        if (mState==StartState || mState==CommaState)
        {
            start();

            std::string name2 = Misc::StringUtils::lowerCase(name);
            char type =  mLocals.getType(name2);
            if (type == 'r')
            {
                int localRefIndex = -1;
                localRefIndex = mLocals.getIndex(name2);
                Generator::pushRef (mCode, mLiterals, localRefIndex);
            }
            else
            {
                if (mSmashCase)
                    Generator::pushString (mCode, mLiterals, Misc::StringUtils::lowerCase (name));
                else
                    Generator::pushString (mCode, mLiterals, name);
            }

            return false;
        }

        return Parser::parseName (name, loc, scanner);
    }

    bool StringParser::parseKeyword (int keyword, const Compiler::TokenLoc& loc, Scanner& scanner)
    {
        if (const Compiler::Extensions *extensions = getContext().getExtensions())
        {
            std::string argumentType; // ignored
            bool hasExplicit = false; // ignored
            if (extensions->isInstruction (keyword, argumentType, hasExplicit))
            {
                // pretend this is not a keyword
                std::string name = loc.mLiteral;
                if (name.size()>=2 && name[0]=='"' && name[name.size()-1]=='"')
                    name = name.substr (1, name.size()-2);
                return parseName (name, loc, scanner);
            }
        }

        return Parser::parseKeyword (keyword, loc, scanner);
    }

    bool StringParser::parseSpecial (int code, const Compiler::TokenLoc& loc, Scanner& scanner)
    {
        if (code==Scanner::S_comma && mState==StartState)
        {
            mState = CommaState;
            return true;
        }

        return Parser::parseSpecial (code, loc, scanner);
    }

    void StringParser::append (std::vector<Interpreter::Type_Code>& code)
    {
        std::copy (mCode.begin(), mCode.end(), std::back_inserter (code));
    }

    void StringParser::reset()
    {
        mState = StartState;
        mCode.clear();
        mSmashCase = false;
        Parser::reset();
    }

    void StringParser::smashCase()
    {
        mSmashCase = true;
    }
}
