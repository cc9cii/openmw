#ifndef TES4COMPILER_JUNKPARSER_H_INCLUDED
#define TES4COMPILER_JUNKPARSER_H_INCLUDED

#include "parser.hpp"

namespace Tes4Compiler
{
    /// \brief Parse an optional single junk token
    class JunkParser : public Parser
    {
            int mIgnoreKeyword;

        public:

            JunkParser (Compiler::ErrorHandler& errorHandler, const Compiler::Context& context,
                int ignoreKeyword = -1);

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
    };
}

#endif
