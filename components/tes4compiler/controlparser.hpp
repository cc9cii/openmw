#ifndef TES4COMPILER_CONTROLPARSER_H_INCLUDED
#define TES4COMPILER_CONTROLPARSER_H_INCLUDED

#include <vector>

#include <components/interpreter/types.hpp>

#include "parser.hpp"
#include "exprparser.hpp"
#include "lineparser.hpp"

namespace Compiler
{
    class Locals;
    class Literals;
}

namespace Tes4Compiler
{
    // Control structure parser

    class ControlParser : public Parser
    {
            enum State
            {
                StartState,
                IfEndState, IfBodyState,
                IfElseifEndState, IfElseifBodyState,
                IfElseEndState, IfElseBodyState,
                IfEndifState,
                WhileEndState, WhileBodyState,
                WhileEndwhileState,
                IfElseJunkState
            };

            typedef std::vector<Interpreter::Type_Code> Codes;
            typedef std::vector<std::pair<Codes, Codes> > IfCodes;

            Compiler::Locals& mLocals;
            Compiler::Literals& mLiterals;
            Codes mCode;
            Codes mCodeBlock;
            IfCodes mIfCode; // condition, body
            LineParser mLineParser;
            ExprParser mExprParser;
            State mState;

            bool parseIfBody (int keyword, const Compiler::TokenLoc& loc, Scanner& scanner);

            bool parseWhileBody (int keyword, const Compiler::TokenLoc& loc, Scanner& scanner);

        public:

            ControlParser (Compiler::ErrorHandler& errorHandler, const Compiler::Context& context,
                Compiler::Locals& locals, Compiler::Literals& literals);

            void appendCode (std::vector<Interpreter::Type_Code>& code) const;
            ///< store generated code in \a code.

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
