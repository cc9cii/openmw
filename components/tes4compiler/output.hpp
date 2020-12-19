#ifndef TES4COMPILER_OUTPUT_H_INCLUDED
#define TES4COMPILER_OUTPUT_H_INCLUDED

#include <vector>

#include <components/interpreter/types.hpp>

#include "../compiler/literals.hpp"

namespace Compiler
{
    class Locals;
}

namespace Tes4Compiler
{
    class Output
    {
            Compiler::Literals mLiterals;
            std::vector<Interpreter::Type_Code> mCode;
            Compiler::Locals& mLocals;

        public:

            Output (Compiler::Locals& locals);

            void getCode (std::vector<Interpreter::Type_Code>& code) const;
            ///< store generated code in \a code.

            const Compiler::Literals& getLiterals() const;

            const Compiler::Locals& getLocals() const;

            const std::vector<Interpreter::Type_Code>& getCode() const;

            Compiler::Literals& getLiterals();

            std::vector<Interpreter::Type_Code>& getCode();

            Compiler::Locals& getLocals();

            void clear(bool keepLocals = false);
    };
}

#endif

