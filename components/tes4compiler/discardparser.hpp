#ifndef TES4COMPILER_DISCARDPARSER_H_INCLUDED
#define TES4COMPILER_DISCARDPARSER_H_INCLUDED

#include "parser.hpp"

namespace Tes4Compiler
{
    /// \brief Parse a single optional numeric value or string and discard it
    class DiscardParser : public Parser
    {
            enum State
            {
                StartState, CommaState, MinusState
            };

            State mState;

        public:

            DiscardParser (Compiler::ErrorHandler& errorHandler, const Compiler::Context& context);

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

            virtual bool parseSpecial (int code, const Compiler::TokenLoc& loc, Scanner& scanner);
            ///< Handle a special character token.
            /// \return fetch another token?

            virtual void reset();
            ///< Reset parser to clean state.
    };
}

#endif

