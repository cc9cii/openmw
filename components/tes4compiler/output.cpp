#include "output.hpp"

#include <cassert>
#include <algorithm>
#include <iterator>

#include "../compiler/locals.hpp"

namespace Tes4Compiler
{
    Output::Output (Compiler::Locals& locals) : mLocals (locals) {}

    void Output::getCode (std::vector<Interpreter::Type_Code>& code) const
    {
        code.clear();

        // header
        code.push_back (static_cast<Interpreter::Type_Code> (mCode.size()));

        assert (mLiterals.getIntegerSize()%4==0);
        code.push_back (static_cast<Interpreter::Type_Code> (mLiterals.getIntegerSize()/4));

        assert (mLiterals.getFloatSize()%4==0);
        code.push_back (static_cast<Interpreter::Type_Code> (mLiterals.getFloatSize()/4));

        assert (mLiterals.getStringSize()%4==0);
        code.push_back (static_cast<Interpreter::Type_Code> (mLiterals.getStringSize()/4));

        // code
        std::copy (mCode.begin(), mCode.end(), std::back_inserter (code));

        // literals
        mLiterals.append (code);
    }

    const Compiler::Literals& Output::getLiterals() const
    {
        return mLiterals;
    }

    const std::vector<Interpreter::Type_Code>& Output::getCode() const
    {
        return mCode;
    }

    const Compiler::Locals& Output::getLocals() const
    {
        return mLocals;
    }

    Compiler::Literals& Output::getLiterals()
    {
        return mLiterals;
    }

    std::vector<Interpreter::Type_Code>& Output::getCode()
    {
        return mCode;
    }

    Compiler::Locals& Output::getLocals()
    {
        return mLocals;
    }

    void Output::clear(bool keepLocals)
    {
        mLiterals.clear();
        mCode.clear();
        if (!keepLocals)
        {
            mLocals.clear();
        }
    }
}

