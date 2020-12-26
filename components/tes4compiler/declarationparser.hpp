#ifndef TES4COMPILER_DECLARATIONPARSER_H_INCLUDED
#define TES4COMPILER_DECLARATIONPARSER_H_INCLUDED

#include "parser.hpp"

namespace Compiler
{
    class Locals;
}

namespace Tes4Compiler
{
    class DeclarationParser : public Parser
    {
            enum State
            {
                StartState, NameState, EndState
            };

            Compiler::Locals& mLocals;
            State mState;
            char mType;

        public:

            DeclarationParser (Compiler::ErrorHandler& errorHandler, const Compiler::Context& context, Compiler::Locals& locals);

            virtual bool parseInt (int value, const Compiler::TokenLoc& loc, Scanner& scanner);

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

            virtual void parseEOF(Scanner& scanner);

            void reset();

    };
}

#endif
