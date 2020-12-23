#include "declarationparser.hpp"

//#include <iostream> // FIXME

#include <components/misc/stringops.hpp>

#include "scanner.hpp"
#include "../compiler/errorhandler.hpp"
#include "skipparser.hpp"
#include "../compiler/locals.hpp"

Tes4Compiler::DeclarationParser::DeclarationParser (Compiler::ErrorHandler& errorHandler,
    const Compiler::Context& context, Compiler::Locals& locals)
: Parser (errorHandler, context), mLocals (locals), mState (State_Begin), mType (0)
{}
// SE02QuestScript has ";" missing as per below (reformatted slightly for legibility)
//
// short metaActiveQuest           0 = not set or both, 1 = arrows, 2 = tears   -- used to "turn on and off" the branches so only one set QTs are up at once
bool Tes4Compiler::DeclarationParser::parseInt (int value, const Compiler::TokenLoc& loc, Scanner& scanner)
{
    getErrorHandler().warning("comment without \";\" character", loc);

    SkipParser skip (getErrorHandler(), getContext());
    scanner.scan (skip);
    mState = State_End;

    return false;
}

bool Tes4Compiler::DeclarationParser::parseName (const std::string& name, const Compiler::TokenLoc& loc,
    Scanner& scanner)
{
    if (mState==State_Name)
    {
        std::string name2 = ::Misc::StringUtils::lowerCase (name);

        char type = mLocals.getType (name2);

        // SE08QuestScript has below:
        //
        // short PasswallBattleBegin ;Set to 1 when Order attacks Passwall after stage 20
        // ...
        // short PasswallBattleBegin ;Set to 1 at the beginning of stage 26
        //
        // Also ArenaGrandChampionMatchScript: short FightOver
        //      Dark05AssassinatedScript: Float fQuestDelayTime
        //      etc
        if (type!=' ')
        {
            /// \todo add option to make re-declared local variables an error
            getErrorHandler().warning ("can't re-declare local variable (ignoring declaration)",
                loc);

            mState = State_End;
            return true;
        }

        mLocals.declare (mType, name2);

        //std::cout << "decl: " << mType << " " << name2 << std::endl; // FIXME: temp testing

        mState = State_End;
        return true;
    }

    return Parser::parseName (name, loc, scanner);
}

bool Tes4Compiler::DeclarationParser::parseKeyword (int keyword, const Compiler::TokenLoc& loc, Scanner& scanner)
{
    if (mState == State_Begin)
    {
        switch (keyword)
        {
            case Scanner::K_short: mType = 's'; break;
            case Scanner::K_long: mType = 'l'; break;
            case Scanner::K_float: mType = 'f'; break;
            case Scanner::K_ref: mType = 'r'; break;
            default: mType = 0;
        }

        if (mType)
        {
            mState = State_Name;
            return true;
        }
    }
    else if (mState==State_Name)
    {
        // allow keywords to be used as local variable names. MW script compiler, you suck!
        /// \todo option to disable this atrocity.
        return parseName (loc.mLiteral, loc, scanner);
    }

    return Parser::parseKeyword (keyword, loc, scanner);
}

bool Tes4Compiler::DeclarationParser::parseSpecial (int code, const Compiler::TokenLoc& loc, Scanner& scanner)
{
    if (code==Scanner::S_newline && mState==State_End)
        return false;

    return Parser::parseSpecial (code, loc, scanner);
}

// some global scripts end with EOF before newline e.g.
// SE39QuestScript, SENQDWildernessScript, SENQDDementiaScript, SENQDManiaScript, SQ01Script, HorseQuestScript,
// ICALLNQDScript, AnvilNQDScript, etc, etc
void Tes4Compiler::DeclarationParser::parseEOF(Scanner& scanner)
{
    // ignore
}

void Tes4Compiler::DeclarationParser::reset()
{
    mState = State_Begin;
}
