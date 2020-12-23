#ifndef TES4COMPILER_SKIPPARSER_H_INCLUDED
#define TES4COMPILER_SKIPPARSER_H_INCLUDED

#include "parser.hpp"

namespace Tes4Compiler
{
    // \brief Skip parser for skipping a line
    //
    // This parser is mainly intended for skipping the rest of a faulty line.

    class SkipParser : public Parser
    {
        public:

            SkipParser (Compiler::ErrorHandler& errorHandler, const Compiler::Context& context);

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

            virtual void SkipParser::parseEOF(Scanner& scanner);
    };
}

#endif

