#ifndef TES4COMPILER_SCRIPTPARSER_H_INCLUDED
#define TES4COMPILER_SCRIPTPARSER_H_INCLUDED


#include "parser.hpp"
#include "lineparser.hpp"
#include "controlparser.hpp"
#include "output.hpp"

namespace Compiler
{
    class Locals;
}

namespace Tes4Compiler
{
    // Script parser, to be used in dialogue scripts and as part of FileParser

    class ScriptParser : public Parser
    {
            Output mOutput;
            LineParser mLineParser;
            ControlParser mControlParser;
            bool mEnd;

        public:

            /// \param end of script is marked by end keyword.
            ScriptParser (Compiler::ErrorHandler& errorHandler, const Compiler::Context& context, Compiler::Locals& locals,
                bool end = false);

            void getCode (std::vector<Interpreter::Type_Code>& code) const;
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

            virtual void parseEOF (Scanner& scanner);
            ///< Handle EOF token.

            void reset(bool keepLocals = false);
            ///< Reset parser to clean state.

            LineParser& getLineParser();

            Compiler::Literals& getLiterals();

            const std::vector<Interpreter::Type_Code>& getCode() const;
    };
}

#endif

