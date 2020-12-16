#ifndef TES4COMPILER_QUICKFILEPARSER_H_INCLUDED
#define TES4COMPILER_QUICKFILEPARSER_H_INCLUDED

#include "parser.hpp"
#include "declarationparser.hpp"

namespace Compiler
{
    class Locals;
}

namespace Tes4Compiler
{
    /// \brief File parser variant that ignores everything but variable declarations
    class QuickFileParser : public Parser
    {
            DeclarationParser mDeclarationParser;

        public:

            QuickFileParser (Compiler::ErrorHandler& errorHandler, const Compiler::Context& context, Compiler::Locals& locals);

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

            virtual void parseEOF (Scanner& scanner);
            ///< Handle EOF token.
    };
}

#endif

