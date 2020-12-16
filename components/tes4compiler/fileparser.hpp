#ifndef TES4COMPILER_FILEPARSER_H_INCLUDED
#define TES4COMPILER_FILEPARSER_H_INCLUDED

#include "parser.hpp"
#include "scriptparser.hpp"
//#include "declarationparser.hpp"
#include "lineparser.hpp"
#include "../compiler/locals.hpp"
#include "../compiler/literals.hpp"

namespace Tes4Compiler
{
    // Top-level parser, to be used for global scripts, local scripts and targeted scripts

    class FileParser : public Parser
    {
            enum State
            {
                StartState, NameState, BeginState, BlockTypeState, BeginCompleteState, EndNameState,
                EndCompleteState
            };

            ScriptParser mScriptParser;
            //DeclarationParser mDeclarationParser;
            LineParser mLineParser;
            State mState;
            std::string mName;
            Compiler::Locals mLocals;

        public:

            FileParser (Compiler::ErrorHandler& errorHandler, Compiler::Context& context);

            std::string getName() const;
            ///< Return script name.

            void getCode (std::vector<Interpreter::Type_Code>& code) const;
            ///< store generated code in \a code.

            const Compiler::Locals& getLocals() const;
            ///< get local variable declarations.

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

            void reset();
            ///< Reset parser to clean state.
    };
}

#endif
