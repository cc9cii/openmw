#ifndef TES4COMPILER_FILEPARSER_H_INCLUDED
#define TES4COMPILER_FILEPARSER_H_INCLUDED

#include <string>
#include <vector>
#include <map>

#include "../compiler/locals.hpp"
#include "../compiler/literals.hpp"
#include "../interpreter/types.hpp"
#include "parser.hpp"
#include "scriptparser.hpp"

namespace Tes4Compiler
{
    // Top-level parser, to be used for global scripts, local scripts and targeted scripts

    class FileParser : public Parser
    {
            enum State
            {
                StartState, NameState, BeginState, BlockTypeState, BeginCompleteState
            };

            ScriptParser mScriptParser;
            State mState;
            std::string mName;
            std::string mBlockType; // temporarily used during the script scanning
            Compiler::Locals mLocals;
            std::map <std::string, std::vector<Interpreter::Type_Code> > mCodeBlocks;

        public:

            FileParser (Compiler::ErrorHandler& errorHandler, Compiler::Context& context);

            const std::string& getName() const;
            ///< Return script name.

            void getCode (std::map<std::string,
                                   std::pair<std::vector<Interpreter::Type_Code>,
                                             Compiler::Locals> >& codes) const;
            ///< store generated code for each block types in \a codes.

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
