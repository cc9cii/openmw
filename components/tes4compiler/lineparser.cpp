#include "lineparser.hpp"

#include <memory>
#include <iostream> // FIXME

#include <components/misc/stringops.hpp>

#include "scanner.hpp"
#include "../compiler/context.hpp"
#include "../compiler/errorhandler.hpp"
#include "skipparser.hpp"
#include "../compiler/locals.hpp"
#include "generator.hpp"
#include "../compiler/extensions.hpp"
#include "declarationparser.hpp"
#include "../compiler/exception.hpp"

namespace Tes4Compiler
{
    void LineParser::parseExpression (Scanner& scanner, const Compiler::TokenLoc& loc)
    {
        mExprParser.reset();

        if (!mExplicit.empty())
        {
            mExprParser.parseName (mExplicit, loc, scanner);
            //if (mState == MemberState)
                //mExprParser.parseSpecial (Scanner::S_member, loc, scanner);
            //else
                //mExprParser.parseSpecial (Scanner::S_ref, loc, scanner);
                mExprParser.parseSpecial (Scanner::S_ref_or_member, loc, scanner);
        }

        scanner.scan (mExprParser);

        char type = mExprParser.append (mCode);
        mState = EndState;

        switch (type)
        {
            case 'l':

                Generator::report (mCode, mLiterals, "%g");
                break;

            case 'f':

                Generator::report (mCode, mLiterals, "%f");
                break;

            default:

                throw std::runtime_error ("unknown expression result type");
        }
    }

    LineParser::LineParser (Compiler::ErrorHandler& errorHandler, const Compiler::Context& context,
        Compiler::Locals& locals, Compiler::Literals& literals, std::vector<Interpreter::Type_Code>& code, bool allowExpression)
    : Parser (errorHandler, context), mLocals (locals), mLiterals (literals), mCode (code),
       mState (StartState), mReferenceMember(false), mButtons(0), mType(0),
       mExprParser (errorHandler, context, locals, literals), mAllowExpression (allowExpression)
    {}

    bool LineParser::parseInt (int value, const Compiler::TokenLoc& loc, Scanner& scanner)
    {
        if (mAllowExpression && mState==StartState)
        {
            scanner.putbackInt (value, loc);
            parseExpression (scanner, loc);
            return true;
        }

        if (mState == MessageButtonState || mState == MessageButtonCommaState)
        {
            // message display seconds parameter - ignore for now


            if (mState != MessageButtonState)
                mState = MessageButtonState; // to trigger the message generation

            return false; // no other tokens expected
        }

        return Parser::parseInt (value, loc, scanner);
    }

    bool LineParser::parseFloat (float value, const Compiler::TokenLoc& loc, Scanner& scanner)
    {
        if (mAllowExpression && mState==StartState)
        {
            scanner.putbackFloat (value, loc);
            parseExpression (scanner, loc);
            return true;
        }

        return Parser::parseFloat (value, loc, scanner);
    }

    bool LineParser::parseName (const std::string& name, const Compiler::TokenLoc& loc,
        Scanner& scanner)
    {
        if (mState==PotentialEndState)
        {
            getErrorHandler().warning ("stray string argument (ignoring it)", loc);
            mState = EndState;
            return true;
        }

        if (mState==SetState)
        {
            std::string name2 = Misc::StringUtils::lowerCase (name);
            mName = name2;

            // local variable?
            char type = mLocals.getType (name2);
            if (type!=' ')
            {
                mType = type;
                mState = SetLocalVarState;
                return true;
            }

            type = getContext().getGlobalType (name2);
            if (type!=' ')
            {
                mType = type;
                mState = SetGlobalVarState;
                return true;
            }

            mState = SetPotentialMemberVarState;
            return true;
        }

        if (mState==SetMemberVarState)
        {
            mMemberName = Misc::StringUtils::lowerCase (name);
            std::pair<char, bool> type = getContext().getMemberType (mMemberName, mName);

            if (type.first!=' ')
            {
                mState = SetMemberVarState2;
                mType = type.first;
                mReferenceMember = type.second;
                return true;
            }

            getErrorHandler().error ("unknown variable", loc);
            SkipParser skip (getErrorHandler(), getContext());
            scanner.scan (skip);
            return false;
        }

        if (mState==MessageState || mState==MessageCommaState)
        {
            std::string arguments;

            for (std::size_t i=0; i<name.size(); ++i)
            {
                if (name[i]=='%')
                {
                    ++i;
                    if (i<name.size())
                    {
                        if (name[i]=='G' || name[i]=='g')
                        {
                            arguments += "l";
                        }
                        else if (name[i]=='S' || name[i]=='s')
                        {
                            arguments += 'S';
                        }
                        else if (name[i]=='.' || name[i]=='f')
                        {
                            arguments += 'f';
                        }
                    }
                }
            }

            if (!arguments.empty())
            {
                mExprParser.reset();
                mExprParser.parseArguments (arguments, scanner, mCode);
            }

            mName = name;
            mButtons = 0;

            mState = MessageButtonState;
            return true;
        }

        if (mState==MessageButtonState || mState==MessageButtonCommaState)
        {
            Generator::pushString (mCode, mLiterals, name);
            mState = MessageButtonState;
            ++mButtons;
            return true;
        }

        if (mState == StartState)
        {
            std::string name2 = Misc::StringUtils::lowerCase (name);

            char type = mLocals.getType (name2);

            // FIXME: if getReference store its FormId as a local ref variable?
            //
            // Hack: getReference returns a formid
            //       if non-zero create a local ref variable with name2
            //
            // alternatively store the FormId as an integer literal? (in both cases)
            // e.g. Generator::pushString(mCode, mLiterals, name2);
            // but this can't work if the local ref variable is filled by a function e.g. GetParentRef
            if (type == 'r' || getContext().getReference(name2) != 0)
            {
                mState = PotentialExplicitState;
                mExplicit = name; // NOTE: name used here because searchViaEditorId is case sensitive

                return true;
            }
        }

        if (mState==StartState && mAllowExpression)
        {
            std::string name2 = Misc::StringUtils::lowerCase (name);

            char type = mLocals.getType (name2);

            if (type!=' ')
            {
                scanner.putbackName (name, loc);
                parseExpression (scanner, loc);
                return true;
            }

            type = getContext().getGlobalType (name2);

            if (type!=' ')
            {
                scanner.putbackName (name, loc);
                parseExpression (scanner, loc);
                return true;
            }
        }

        return Parser::parseName (name, loc, scanner);
    }

    bool LineParser::parseKeyword (int keyword, const Compiler::TokenLoc& loc, Scanner& scanner)
    {
        if (mState==MessageState || mState==MessageCommaState)
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
        }

        if (mState==SetMemberVarState)
        {
            mMemberName = loc.mLiteral;
            std::pair<char, bool> type = getContext().getMemberType (mMemberName, mName);

            if (type.first!=' ')
            {
                mState = SetMemberVarState2;
                mType = type.first;
                mReferenceMember = type.second;
                return true;
            }
        }

        if (mState==SetPotentialMemberVarState && keyword==Scanner::K_to)
        {
            getErrorHandler().warning ("unknown variable (ignoring set instruction)", loc);
            SkipParser skip (getErrorHandler(), getContext());
            scanner.scan (skip);
            return false;
        }

        if (mState==SetState)
        {
            // allow keywords to be used as variable names when assigning a value to a variable.
            return parseName (loc.mLiteral, loc, scanner);
        }

        if (mState==StartState || mState==ExplicitState)
        {
            switch (keyword)
            {
                case Scanner::K_enable:

                    Generator::enable (mCode, mLiterals, mExplicit);
                    mState = PotentialEndState;
                    return true;

                case Scanner::K_disable:

                    Generator::disable (mCode, mLiterals, mExplicit);
                    mState = PotentialEndState;
                    return true;
            }

            // check for custom extensions
            if (const Compiler::Extensions *extensions = getContext().getExtensions())
            {
                char returnType;
                std::string argumentType;

                bool hasExplicit = mState==ExplicitState;
                if (extensions->isInstruction (keyword, argumentType, hasExplicit))
                {
                    if (!hasExplicit && mState==ExplicitState)
                    {
                        getErrorHandler().warning ("stray explicit reference (ignoring it)", loc);
                        mExplicit.clear();
                    }

// FIXME: does this occur in TES4?
#if 1
                    try
                    {
                        // workaround for broken positioncell instructions.
                        /// \todo add option to disable this
                        std::auto_ptr<Compiler::ErrorDowngrade> errorDowngrade (0);
                        if (Misc::StringUtils::lowerCase (loc.mLiteral)=="positioncell")
                            errorDowngrade.reset (new Compiler::ErrorDowngrade (getErrorHandler()));
#endif

                        std::vector<Interpreter::Type_Code> code;
                        int optionals = mExprParser.parseArguments (argumentType, scanner, code, keyword);

                        //std::cout << "instruction " << loc.mLiteral << std::endl; // FIXME: temp testing

                        mCode.insert (mCode.end(), code.begin(), code.end());
                        extensions->generateInstructionCode (keyword, mCode, mLiterals,
                            mExplicit, optionals);
#if 1
                    }
                    catch (const Compiler::SourceException&)
                    {
                        // Ignore argument exceptions for positioncell.
                        /// \todo add option to disable this
                        if (Misc::StringUtils::lowerCase (loc.mLiteral)=="positioncell")
                        {
                            SkipParser skip (getErrorHandler(), getContext());
                            scanner.scan (skip);
                            return false;
                        }

                        throw;
                    }
#endif

                    mState = EndState;
                    return true;
                }
                else if (extensions->isFunction(keyword, returnType, argumentType, hasExplicit))
                {
                    scanner.putbackKeyword(keyword, loc);
                    parseExpression(scanner, loc);

                    mState = EndState;
                    return true;
                }
            }

            if (mAllowExpression)
            {
                if (keyword==Scanner::K_getdisabled || keyword==Scanner::K_getdistance)
                {
                    scanner.putbackKeyword (keyword, loc);
                    parseExpression (scanner, loc);
                    mState = EndState;
                    return true;
                }

                if (const Compiler::Extensions *extensions = getContext().getExtensions())
                {
                    char returnType;
                    std::string argumentType;

                    bool hasExplicit = !mExplicit.empty();

                    if (extensions->isFunction (keyword, returnType, argumentType, hasExplicit))
                    {
                        if (!hasExplicit && !mExplicit.empty())
                        {
                            getErrorHandler().warning ("stray explicit reference (ignoring it)", loc);
                            mExplicit.clear();
                        }

                        scanner.putbackKeyword (keyword, loc);
                        parseExpression (scanner, loc);
                        mState = EndState;
                        return true;
                    }
                }
            }
        }

        if (mState==ExplicitState)
        {
            // drop stray explicit reference
            getErrorHandler().warning ("stray explicit reference (ignoring it)", loc);
            mState = StartState;
            mExplicit.clear();
        }

        if (mState==StartState)
        {
            switch (keyword)
            {
                case Scanner::K_short:
                case Scanner::K_long:
                case Scanner::K_int: // SE06SCRIPT uses "int"
                case Scanner::K_float:
                case Scanner::K_ref:
                {
                    if (!getContext().canDeclareLocals())
                    {
                        getErrorHandler().error (
                            "local variables can't be declared in this context", loc);
                        SkipParser skip (getErrorHandler(), getContext());
                        scanner.scan (skip);
                        return true;
                    }

                    DeclarationParser declaration (getErrorHandler(), getContext(), mLocals);
                    if (declaration.parseKeyword (keyword, loc, scanner))
                        scanner.scan (declaration);

                    return false;
                }

                case Scanner::K_set: mState = SetState; return true;

                case Scanner::K_messagebox:
                case Scanner::K_message:
                {
                    mState = MessageState; return true;
                }

                case Scanner::K_return:

                    Generator::exit (mCode);
                    mState = EndState;
                    return true;

                case Scanner::K_stopscript:

                    mExprParser.parseArguments ("c", scanner, mCode);
                    Generator::stopScript (mCode);
                    mState = EndState;
                    return true;

                case Scanner::K_else:

                    getErrorHandler().warning ("stray else (ignoring it)", loc);
                    mState = EndState;
                    return true;

                case Scanner::K_endif:
                //case Scanner::K_endif_broken: // MG09Script

                    getErrorHandler().warning ("stray endif (ignoring it)", loc);
                    mState = EndState;
                    return true;

                case Scanner::K_begin:

                    getErrorHandler().warning ("stray begin (ignoring it)", loc);
                    mState = EndState;
                    return true;
            }
        }
        else if (mState==SetLocalVarState && keyword==Scanner::K_to)
        {
            mExprParser.reset();
            scanner.scan (mExprParser);

            std::vector<Interpreter::Type_Code> code;
            char type = mExprParser.append (code);

            Generator::assignToLocal (mCode, mLocals.getType (mName),
                mLocals.getIndex (mName), code, type);

            mState = EndState;
            return true;
        }
        else if (mState==SetGlobalVarState && keyword==Scanner::K_to)
        {
            mExprParser.reset();
            scanner.scan (mExprParser);

            std::vector<Interpreter::Type_Code> code;
            char type = mExprParser.append (code);

            Generator::assignToGlobal (mCode, mLiterals, mType, mName, code, type);

            mState = EndState;
            return true;
        }
        else if (mState==SetMemberVarState2 && keyword==Scanner::K_to)
        {
            mExprParser.reset();
            scanner.scan (mExprParser);

            std::vector<Interpreter::Type_Code> code;
            char type = mExprParser.append (code);

            Generator::assignToMember (mCode, mLiterals, mType, mMemberName, mName, code, type,
                !mReferenceMember);

            mState = EndState;
            return true;
        }

        if (mAllowExpression)
        {
            if (keyword==Scanner::K_getsquareroot || //keyword==Scanner::K_menumode ||
                keyword==Scanner::K_random || keyword==Scanner::K_scriptrunning ||
                keyword==Scanner::K_getsecondspassed)
            {
                scanner.putbackKeyword (keyword, loc);
                parseExpression (scanner, loc);
                mState = EndState;
                return true;
            }
        }

        return Parser::parseKeyword (keyword, loc, scanner);
    }

    bool LineParser::parseSpecial (int code, const Compiler::TokenLoc& loc, Scanner& scanner)
    {
        if (mState==EndState && code==Scanner::S_open)
        {
            getErrorHandler().warning ("stray '[' or '(' at the end of the line (ignoring it)",
                loc);
            return true;
        }

        if (code==Scanner::S_newline &&
            (mState==EndState || mState==StartState || mState==PotentialEndState))
            return false;

        if (code==Scanner::S_comma && mState==MessageState)
        {
            mState = MessageCommaState;
            return true;
        }

        if (code==Scanner::S_ref_or_member && mState==PotentialExplicitState)
        {
            mState = ExplicitState;
            return true;
        }

#if 0
        if (code==Scanner::S_member && mState==PotentialExplicitState)
        {
            mState = MemberState;
            parseExpression (scanner, loc);
            mState = EndState;
            return true;
        }
#endif
        if (code==Scanner::S_newline && mState==MessageButtonState)
        {
            Generator::message (mCode, mLiterals, mName, mButtons);
            return false;
        }

        if (code==Scanner::S_comma && mState==MessageButtonState)
        {
            mState = MessageButtonCommaState;
            return true;
        }

        if (code == Scanner::S_ref_or_member && mState == SetPotentialMemberVarState)
        {
            mState = SetMemberVarState;
            return true;
        }

        if (mAllowExpression && mState==StartState &&
            (code==Scanner::S_open || code==Scanner::S_minus || code==Scanner::S_plus))
        {
            scanner.putbackSpecial (code, loc);
            parseExpression (scanner, loc);
            mState = EndState;
            return true;
        }

        return Parser::parseSpecial (code, loc, scanner);
    }

    void LineParser::reset()
    {
        mState = StartState;
        mName.clear();
        mExplicit.clear();
    }
}
