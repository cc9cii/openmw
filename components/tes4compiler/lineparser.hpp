#ifndef TES4COMPILER_LINEPARSER_H_INCLUDED
#define TES4COMPILER_LINEPARSER_H_INCLUDED

#include <vector>

#include <components/interpreter/types.hpp>

#include "parser.hpp"
#include "exprparser.hpp"

namespace Compiler
{
    class Locals;
    class Literals;
}

namespace Tes4Compiler
{
    /// \brief Line parser, to be used in console scripts and as part of ScriptParser

    class LineParser : public Parser
    {
            enum State
            {
                StartState,
                SetState, SetLocalVarState, SetGlobalVarState, SetPotentialMemberVarState,
                SetMemberVarState, SetMemberVarState2,
                MessageState, MessageCommaState, MessageButtonState, MessageButtonCommaState,
                EndState, PotentialEndState /* may have a stray string argument */,
                PotentialExplicitState, ExplicitState//, MemberState
            };

            Compiler::Locals& mLocals;
            Compiler::Literals& mLiterals;
            std::vector<Interpreter::Type_Code>& mCode;
            State mState;
            std::string mName;
            std::string mMemberName;
            bool mReferenceMember;
            int mButtons;
            std::string mExplicit;
            char mType;
            ExprParser mExprParser;
            bool mAllowExpression;

            void parseExpression (Scanner& scanner, const Compiler::TokenLoc& loc);

        public:

            LineParser (Compiler::ErrorHandler& errorHandler, const Compiler::Context& context,
                Compiler::Locals& locals, Compiler::Literals& literals, std::vector<Interpreter::Type_Code>& code,
                bool allowExpression = false);
            ///< \param allowExpression Allow lines consisting of a naked expression
            /// (result is send to the messagebox interface)

            virtual bool parseInt (int value, const Compiler::TokenLoc& loc, Scanner& scanner);
            ///< Handle an int token.
            /// \return fetch another token?

            virtual bool parseFloat (float value, const Compiler::TokenLoc& loc, Scanner& scanner);
            ///< Handle a float token.
            /// \return fetch another token?

            virtual bool parseName (const std::string& name, const Compiler::TokenLoc& loc,
                Scanner& scanner);
            ///< Handle a name token.
            /// \return fetch another token?

            virtual bool parseKeyword (int keyword, const Compiler::TokenLoc& loc, Scanner& scanner);
            ///< Handle a keyword token.
            /// \return fetch another token?

            virtual bool parseSpecial (int code, const Compiler::TokenLoc& loc, Scanner& scanner);
            ///< Handle a special character token.
            /// \return fetch another token?

            void reset();
            ///< Reset parser to clean state.
    };
}

#endif
