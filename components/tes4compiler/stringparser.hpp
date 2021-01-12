#ifndef TES$COMPILER_STRINGPARSER_H_INCLUDED
#define TES4COMPILER_STRINGPARSER_H_INCLUDED

#include <vector>

#include <components/interpreter/types.hpp>

#include "parser.hpp"

namespace Compiler
{
    class Locals;
    class Literals;
}

namespace Tes4Compiler
{
    class StringParser : public Parser
    {
            enum State
            {
                StartState, CommaState
            };

            Compiler::Locals& mLocals;
            Compiler::Literals& mLiterals;
            State mState;
            std::vector<Interpreter::Type_Code> mCode;
            bool mSmashCase;

        public:

            StringParser (Compiler::ErrorHandler& errorHandler, const Compiler::Context& context, Compiler::Locals& locals, Compiler::Literals& literals);

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

            void append (std::vector<Interpreter::Type_Code>& code);
            ///< Append code for parsed string.

            void smashCase();
            ///< Transform all scanned strings to lower case

            void reset();
            ///< Reset parser to clean state (this includes the smashCase function).
    };
}

#endif
