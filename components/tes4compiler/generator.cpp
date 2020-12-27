#include "generator.hpp"

#include <cassert>
#include <algorithm>
#include <iterator>
#include <stdexcept>
#include <iostream> // FIXME

#include "../compiler/literals.hpp"

namespace
{
    void opPushInt (Tes4Compiler::Generator::CodeContainer& code, int value)
    {
        code.push_back (Tes4Compiler::Generator::segment0 (0, value));
    }

    void opFetchIntLiteral (Tes4Compiler::Generator::CodeContainer& code)
    {
        code.push_back (Tes4Compiler::Generator::segment5 (4));
    }

    void opFetchFloatLiteral (Tes4Compiler::Generator::CodeContainer& code)
    {
        code.push_back (Tes4Compiler::Generator::segment5 (5));
    }

    void opIntToFloat (Tes4Compiler::Generator::CodeContainer& code)
    {
        code.push_back (Tes4Compiler::Generator::segment5 (3));
    }

    void opFloatToInt (Tes4Compiler::Generator::CodeContainer& code)
    {
        code.push_back (Tes4Compiler::Generator::segment5 (6));
    }

    void opStoreLocalShort (Tes4Compiler::Generator::CodeContainer& code)
    {
        code.push_back (Tes4Compiler::Generator::segment5 (0));
    }

    void opStoreLocalLong (Tes4Compiler::Generator::CodeContainer& code)
    {
        code.push_back (Tes4Compiler::Generator::segment5 (1));
    }

    void opStoreLocalFloat (Tes4Compiler::Generator::CodeContainer& code)
    {
        code.push_back (Tes4Compiler::Generator::segment5 (2));
    }

    void opStoreLocalRef (Tes4Compiler::Generator::CodeContainer& code)
    {
        code.push_back (Tes4Compiler::Generator::segment5 (75));
    }

    void opNegateInt (Tes4Compiler::Generator::CodeContainer& code)
    {
        code.push_back (Tes4Compiler::Generator::segment5 (7));
    }

    void opNegateFloat (Tes4Compiler::Generator::CodeContainer& code)
    {
        code.push_back (Tes4Compiler::Generator::segment5 (8));
    }

    void opAddInt (Tes4Compiler::Generator::CodeContainer& code)
    {
        code.push_back (Tes4Compiler::Generator::segment5 (9));
    }

    void opAddFloat (Tes4Compiler::Generator::CodeContainer& code)
    {
        code.push_back (Tes4Compiler::Generator::segment5 (10));
    }

    void opSubInt (Tes4Compiler::Generator::CodeContainer& code)
    {
        code.push_back (Tes4Compiler::Generator::segment5 (11));
    }

    void opSubFloat (Tes4Compiler::Generator::CodeContainer& code)
    {
        code.push_back (Tes4Compiler::Generator::segment5 (12));
    }

    void opMulInt (Tes4Compiler::Generator::CodeContainer& code)
    {
        code.push_back (Tes4Compiler::Generator::segment5 (13));
    }

    void opMulFloat (Tes4Compiler::Generator::CodeContainer& code)
    {
        code.push_back (Tes4Compiler::Generator::segment5 (14));
    }

    void opDivInt (Tes4Compiler::Generator::CodeContainer& code)
    {
        code.push_back (Tes4Compiler::Generator::segment5 (15));
    }

    void opDivFloat (Tes4Compiler::Generator::CodeContainer& code)
    {
        code.push_back (Tes4Compiler::Generator::segment5 (16));
    }

    void opIntToFloat1 (Tes4Compiler::Generator::CodeContainer& code)
    {
        code.push_back (Tes4Compiler::Generator::segment5 (17));
    }

    void opSquareRoot (Tes4Compiler::Generator::CodeContainer& code)
    {
        code.push_back (Tes4Compiler::Generator::segment5 (19));
    }

    void opReturn (Tes4Compiler::Generator::CodeContainer& code)
    {
        code.push_back (Tes4Compiler::Generator::segment5 (20));
    }

    void opMessageBox (Tes4Compiler::Generator::CodeContainer& code, int buttons)
    {
        code.push_back (Tes4Compiler::Generator::segment3 (0, buttons));
    }

    void opReport (Tes4Compiler::Generator::CodeContainer& code)
    {
        code.push_back (Tes4Compiler::Generator::segment5 (58));
    }

    void opFetchLocalShort (Tes4Compiler::Generator::CodeContainer& code)
    {
        code.push_back (Tes4Compiler::Generator::segment5 (21));
    }

    void opFetchLocalLong (Tes4Compiler::Generator::CodeContainer& code)
    {
        code.push_back (Tes4Compiler::Generator::segment5 (22));
    }

    void opFetchLocalFloat (Tes4Compiler::Generator::CodeContainer& code)
    {
        code.push_back (Tes4Compiler::Generator::segment5 (23));
    }

    void opFetchLocalRef (Tes4Compiler::Generator::CodeContainer& code)
    {
        code.push_back (Tes4Compiler::Generator::segment5 (74));
    }

    void opJumpForward (Tes4Compiler::Generator::CodeContainer& code, int offset)
    {
        code.push_back (Tes4Compiler::Generator::segment0 (1, offset));
    }

    void opJumpBackward (Tes4Compiler::Generator::CodeContainer& code, int offset)
    {
        code.push_back (Tes4Compiler::Generator::segment0 (2, offset));
    }

    /*
    Currently unused
    void opSkipOnZero (Tes4Compiler::Generator::CodeContainer& code)
    {
        code.push_back (Tes4Compiler::Generator::segment5 (24));
    }
    */

    void opSkipOnNonZero (Tes4Compiler::Generator::CodeContainer& code)
    {
        code.push_back (Tes4Compiler::Generator::segment5 (25));
    }

    void opEqualInt (Tes4Compiler::Generator::CodeContainer& code)
    {
        code.push_back (Tes4Compiler::Generator::segment5 (26));
    }

    void opNonEqualInt (Tes4Compiler::Generator::CodeContainer& code)
    {
        code.push_back (Tes4Compiler::Generator::segment5 (27));
    }

    void opLessThanInt (Tes4Compiler::Generator::CodeContainer& code)
    {
        code.push_back (Tes4Compiler::Generator::segment5 (28));
    }

    void opLessOrEqualInt (Tes4Compiler::Generator::CodeContainer& code)
    {
        code.push_back (Tes4Compiler::Generator::segment5 (29));
    }

    void opGreaterThanInt (Tes4Compiler::Generator::CodeContainer& code)
    {
        code.push_back (Tes4Compiler::Generator::segment5 (30));
    }

    void opGreaterOrEqualInt (Tes4Compiler::Generator::CodeContainer& code)
    {
        code.push_back (Tes4Compiler::Generator::segment5 (31));
    }

    void opEqualFloat (Tes4Compiler::Generator::CodeContainer& code)
    {
        code.push_back (Tes4Compiler::Generator::segment5 (32));
    }

    void opNonEqualFloat (Tes4Compiler::Generator::CodeContainer& code)
    {
        code.push_back (Tes4Compiler::Generator::segment5 (33));
    }

    void opLessThanFloat (Tes4Compiler::Generator::CodeContainer& code)
    {
        code.push_back (Tes4Compiler::Generator::segment5 (34));
    }

    void opLessOrEqualFloat (Tes4Compiler::Generator::CodeContainer& code)
    {
        code.push_back (Tes4Compiler::Generator::segment5 (35));
    }

    void opGreaterThanFloat (Tes4Compiler::Generator::CodeContainer& code)
    {
        code.push_back (Tes4Compiler::Generator::segment5 (36));
    }

    void opGreaterOrEqualFloat (Tes4Compiler::Generator::CodeContainer& code)
    {
        code.push_back (Tes4Compiler::Generator::segment5 (37));
    }

    void opMenuMode (Tes4Compiler::Generator::CodeContainer& code)
    {
        code.push_back (Tes4Compiler::Generator::segment5 (38));
    }

    void opStoreGlobalShort (Tes4Compiler::Generator::CodeContainer& code)
    {
        code.push_back (Tes4Compiler::Generator::segment5 (39));
    }

    void opStoreGlobalLong (Tes4Compiler::Generator::CodeContainer& code)
    {
        code.push_back (Tes4Compiler::Generator::segment5 (40));
    }

    void opStoreGlobalFloat (Tes4Compiler::Generator::CodeContainer& code)
    {
        code.push_back (Tes4Compiler::Generator::segment5 (41));
    }

    void opFetchGlobalShort (Tes4Compiler::Generator::CodeContainer& code)
    {
        code.push_back (Tes4Compiler::Generator::segment5 (42));
    }

    void opFetchGlobalLong (Tes4Compiler::Generator::CodeContainer& code)
    {
        code.push_back (Tes4Compiler::Generator::segment5 (43));
    }

    void opFetchGlobalFloat (Tes4Compiler::Generator::CodeContainer& code)
    {
        code.push_back (Tes4Compiler::Generator::segment5 (44));
    }
#if 0
    void opStoreMemberShort (Tes4Compiler::Generator::CodeContainer& code, bool global)
    {
        code.push_back (Tes4Compiler::Generator::segment5 (global ? 65 : 59));
    }

    void opStoreMemberLong (Tes4Compiler::Generator::CodeContainer& code, bool global)
    {
        code.push_back (Tes4Compiler::Generator::segment5 (global ? 66 : 60));
    }

    void opStoreMemberFloat (Tes4Compiler::Generator::CodeContainer& code, bool global)
    {
        code.push_back (Tes4Compiler::Generator::segment5 (global ? 67 : 61));
    }

    void opFetchMemberShort (Tes4Compiler::Generator::CodeContainer& code, bool global)
    {
        code.push_back (Tes4Compiler::Generator::segment5 (global ? 68 : 62));
    }

    void opFetchMemberLong (Tes4Compiler::Generator::CodeContainer& code, bool global)
    {
        code.push_back (Tes4Compiler::Generator::segment5 (global ? 69 : 63));
    }

    void opFetchMemberFloat (Tes4Compiler::Generator::CodeContainer& code, bool global)
    {
        code.push_back (Tes4Compiler::Generator::segment5 (global ? 70 : 64));
    }
#else
    // NOTE: here global means quest script
    void opStoreScriptMemberShort (Tes4Compiler::Generator::CodeContainer& code, bool global)
    {
        code.push_back (Tes4Compiler::Generator::segment5 (global ? 84 : 76));
    }

    void opStoreScriptMemberLong (Tes4Compiler::Generator::CodeContainer& code, bool global)
    {
        code.push_back (Tes4Compiler::Generator::segment5 (global ? 85 : 77));
    }

    void opStoreScriptMemberFloat (Tes4Compiler::Generator::CodeContainer& code, bool global)
    {
        code.push_back (Tes4Compiler::Generator::segment5 (global ? 86 : 78));
    }

    void opStoreScriptMemberRef (Tes4Compiler::Generator::CodeContainer& code, bool global)
    {
        code.push_back (Tes4Compiler::Generator::segment5 (global ? 87 : 79));
    }

    void opFetchScriptMemberShort (Tes4Compiler::Generator::CodeContainer& code, bool global)
    {
        code.push_back (Tes4Compiler::Generator::segment5 (global ? 88 : 80));
    }

    void opFetchScriptMemberLong (Tes4Compiler::Generator::CodeContainer& code, bool global)
    {
        code.push_back (Tes4Compiler::Generator::segment5 (global ? 89 : 81));
    }

    void opFetchScriptMemberFloat (Tes4Compiler::Generator::CodeContainer& code, bool global)
    {
        code.push_back (Tes4Compiler::Generator::segment5 (global ? 90 : 82));
    }

    void opFetchScriptMemberRef (Tes4Compiler::Generator::CodeContainer& code, bool global)
    {
        code.push_back (Tes4Compiler::Generator::segment5 (global ? 91 : 83));
    }
#endif
    void opRandom (Tes4Compiler::Generator::CodeContainer& code)
    {
        code.push_back (Tes4Compiler::Generator::segment5 (45));
    }

    void opScriptRunning (Tes4Compiler::Generator::CodeContainer& code)
    {
        code.push_back (Tes4Compiler::Generator::segment5 (46));
    }

    void opStartScript (Tes4Compiler::Generator::CodeContainer& code, bool targeted)
    {
        code.push_back (Tes4Compiler::Generator::segment5 (targeted ? 71 : 47));
    }

    void opStopScript (Tes4Compiler::Generator::CodeContainer& code)
    {
        code.push_back (Tes4Compiler::Generator::segment5 (48));
    }

    void opGetDistance (Tes4Compiler::Generator::CodeContainer& code)
    {
        code.push_back (Tes4Compiler::Generator::segment5 (92));
    }

    void opGetSecondsPassed (Tes4Compiler::Generator::CodeContainer& code)
    {
        code.push_back (Tes4Compiler::Generator::segment5 (50));
    }

    void opEnable (Tes4Compiler::Generator::CodeContainer& code)
    {
        code.push_back (Tes4Compiler::Generator::segment5 (51));
    }

    void opDisable (Tes4Compiler::Generator::CodeContainer& code)
    {
        code.push_back (Tes4Compiler::Generator::segment5 (52));
    }

    void opGetDisabled (Tes4Compiler::Generator::CodeContainer& code)
    {
        code.push_back (Tes4Compiler::Generator::segment5 (53));
    }

    void opEnableExplicit (Tes4Compiler::Generator::CodeContainer& code)
    {
        code.push_back (Tes4Compiler::Generator::segment5 (54));
    }

    void opDisableExplicit (Tes4Compiler::Generator::CodeContainer& code)
    {
        code.push_back (Tes4Compiler::Generator::segment5 (55));
    }

    void opGetDisabledExplicit (Tes4Compiler::Generator::CodeContainer& code)
    {
        code.push_back (Tes4Compiler::Generator::segment5 (56));
    }

    void opGetDistanceExplicit (Tes4Compiler::Generator::CodeContainer& code)
    {
        code.push_back (Tes4Compiler::Generator::segment5 (93));
    }

    void opLogicalAndInt (Tes4Compiler::Generator::CodeContainer& code)
    {
        code.push_back (Tes4Compiler::Generator::segment5 (72));
    }

    void opLogicalOrInt (Tes4Compiler::Generator::CodeContainer& code)
    {
        code.push_back (Tes4Compiler::Generator::segment5 (73));
    }
}

namespace Tes4Compiler
{
    namespace Generator
    {
        void pushInt (CodeContainer& code, Compiler::Literals& literals, int value)
        {
            int index = literals.addInteger (value);
            //std::cout << "generate push int " << std::dec << value << " index " << index << std::endl; // FIXME: temp testing
            opPushInt (code, index);
            opFetchIntLiteral (code);
        }

        void pushFloat (CodeContainer& code, Compiler::Literals& literals, float value)
        {
            int index = literals.addFloat (value);
            opPushInt (code, index);
            opFetchFloatLiteral (code);
        }

        void pushString (CodeContainer& code, Compiler::Literals& literals, const std::string& value)
        {
            int index = literals.addString (value);
            //std::cout << "generate push string " << value << " index " << index << std::endl; // FIXME: temp testing
            opPushInt (code, index);
        }

        void assignToLocal (CodeContainer& code, char localType,
            int localIndex, const CodeContainer& value, char valueType)
        {
            opPushInt (code, localIndex);

            std::copy (value.begin(), value.end(), std::back_inserter (code));

            if (localType!=valueType)
            {
                if (localType=='f' && valueType=='l')
                {
                    opIntToFloat (code);
                }
                else if ((localType=='l' || localType=='s') && valueType=='f')
                {
                    opFloatToInt (code);
                }
            }

            switch (localType)
            {
                case 'f':

                    opStoreLocalFloat (code);
                    break;

                case 's':

                    opStoreLocalShort (code);
                    break;

                case 'l':

                    opStoreLocalLong (code);
                    break;

                case 'r':

                    opStoreLocalRef(code);
                    break;

                default:

                    assert (0);
            }
        }

        void negate (CodeContainer& code, char valueType)
        {
            switch (valueType)
            {
                case 'l':

                    opNegateInt (code);
                    break;

                case 'f':

                    opNegateFloat (code);
                    break;

                default:

                    assert (0);
            }
        }

        void add (CodeContainer& code, char valueType1, char valueType2)
        {
            if (valueType1=='l' && valueType2=='l')
            {
                opAddInt (code);
            }
            else
            {
                if (valueType1=='l')
                    opIntToFloat1 (code);

                if (valueType2=='l')
                    opIntToFloat (code);

                opAddFloat (code);
            }
        }

        void sub (CodeContainer& code, char valueType1, char valueType2)
        {
            if (valueType1=='l' && valueType2=='l')
            {
                opSubInt (code);
            }
            else
            {
                if (valueType1=='l')
                    opIntToFloat1 (code);

                if (valueType2=='l')
                    opIntToFloat (code);

                opSubFloat (code);
            }
        }

        void mul (CodeContainer& code, char valueType1, char valueType2)
        {
            if (valueType1=='l' && valueType2=='l')
            {
                opMulInt (code);
            }
            else
            {
                if (valueType1=='l')
                    opIntToFloat1 (code);

                if (valueType2=='l')
                    opIntToFloat (code);

                opMulFloat (code);
            }
        }

        void div (CodeContainer& code, char valueType1, char valueType2)
        {
            if (valueType1=='l' && valueType2=='l')
            {
                opDivInt (code);
            }
            else
            {
                if (valueType1=='l')
                    opIntToFloat1 (code);

                if (valueType2=='l')
                    opIntToFloat (code);

                opDivFloat (code);
            }
        }

        void boolean_and (CodeContainer& code, char valueType1, char valueType2)
        {
            if (valueType1=='l' && valueType2=='l')
            {
                opLogicalAndInt (code);
                //std::cout << "generate boolean and" << std::endl; // FIXME: temp testing
            }
            else
                throw std::logic_error ("illegal boolean type");
        }

        void boolean_or (CodeContainer& code, char valueType1, char valueType2)
        {
            if (valueType1=='l' && valueType2=='l')
            {
                opLogicalOrInt (code);
                //std::cout << "generate boolean or" << std::endl; // FIXME: temp testing
            }
            else
                throw std::logic_error ("illegal boolean type");
        }

        void convert (CodeContainer& code, char fromType, char toType)
        {
            if (fromType!=toType)
            {
                if (fromType=='f' && toType=='l')
                    opFloatToInt (code);
                else if (fromType=='l' && toType=='f')
                    opIntToFloat (code);
                else
                    throw std::logic_error ("illegal type conversion");
            }
        }

        void squareRoot (CodeContainer& code)
        {
            opSquareRoot (code);
        }

        void exit (CodeContainer& code)
        {
            opReturn (code);
        }

        void message (CodeContainer& code, Compiler::Literals& literals, const std::string& message,
            int buttons)
        {
            assert (buttons>=0);

            if (buttons>=256)
                throw std::runtime_error ("A message box can't have more than 255 buttons");

            int index = literals.addString (message);

            opPushInt (code, index);
            opMessageBox (code, buttons);
        }

        void report (CodeContainer& code, Compiler::Literals& literals, const std::string& message)
        {
            int index = literals.addString (message);

            opPushInt (code, index);
            opReport (code);
        }

        void fetchLocal (CodeContainer& code, char localType, int localIndex)
        {
            opPushInt (code, localIndex);

            switch (localType)
            {
                case 'f':

                    opFetchLocalFloat (code);
                    break;

                case 's':

                    opFetchLocalShort (code);
                    break;

                case 'l':

                    opFetchLocalLong (code);
                    break;

                case 'r':

                    opFetchLocalRef (code);
                    break;

                default:

                    assert (0);
            }
        }

        void jump (CodeContainer& code, int offset)
        {
            if (offset>0)
                opJumpForward (code, offset);
            else if (offset<0)
                opJumpBackward (code, -offset);
            else
                throw std::logic_error ("infinite loop");
        }

        void jumpOnZero (CodeContainer& code, int offset)
        {
            opSkipOnNonZero (code);

            if (offset<0)
                --offset; // compensate for skip instruction

            jump (code, offset);
        }

        void compare (CodeContainer& code, char op, char valueType1, char valueType2)
        {
            //std::cout << "generate compare " << op << std::endl; // FIXME: temp testing

            if (valueType1=='l' && valueType2=='l')
            {
                switch (op)
                {
                    case 'e': opEqualInt (code); break;
                    case 'n': opNonEqualInt (code); break;
                    case 'l': opLessThanInt (code); break;
                    case 'L': opLessOrEqualInt (code); break;
                    case 'g': opGreaterThanInt (code); break;
                    case 'G': opGreaterOrEqualInt (code); break;

                    default:

                        assert (0);
                }
            }
            else
            {
                if (valueType1=='l')
                    opIntToFloat1 (code);

                if (valueType2=='l')
                    opIntToFloat (code);

                switch (op)
                {
                    case 'e': opEqualFloat (code); break;
                    case 'n': opNonEqualFloat (code); break;
                    case 'l': opLessThanFloat (code); break;
                    case 'L': opLessOrEqualFloat (code); break;
                    case 'g': opGreaterThanFloat (code); break;
                    case 'G': opGreaterOrEqualFloat (code); break;

                    default:

                        assert (0);
                }
            }
        }

        void menuMode (CodeContainer& code)
        {
            opMenuMode (code);
        }

        void assignToGlobal (CodeContainer& code, Compiler::Literals& literals, char localType,
            const std::string& name, const CodeContainer& value, char valueType)
        {
            int index = literals.addString (name);

            opPushInt (code, index);

            std::copy (value.begin(), value.end(), std::back_inserter (code));

            if (localType!=valueType)
            {
                if (localType=='f' && (valueType=='l' || valueType=='s'))
                {
                    opIntToFloat (code);
                }
                else if ((localType=='l' || localType=='s') && valueType=='f')
                {
                    opFloatToInt (code);
                }
            }

            switch (localType)
            {
                case 'f':

                    opStoreGlobalFloat (code);
                    break;

                case 's':

                    opStoreGlobalShort (code);
                    break;

                case 'l':

                    opStoreGlobalLong (code);
                    break;

                default:

                    assert (0);
            }
        }

        void fetchGlobal (CodeContainer& code, Compiler::Literals& literals, char localType,
            const std::string& name)
        {
            int index = literals.addString (name);

            opPushInt (code, index);

            switch (localType)
            {
                case 'f':

                    opFetchGlobalFloat (code);
                    break;

                case 's':

                    opFetchGlobalShort (code);
                    break;

                case 'l':

                    opFetchGlobalLong (code);
                    break;

                default:

                    assert (0);
            }
        }

        void assignToMember (CodeContainer& code, Compiler::Literals& literals, char localType,
            const std::string& name, const std::string& id, const CodeContainer& value,
            char valueType, bool global)
        {
            int index = literals.addString (name);

            opPushInt (code, index);

            index = literals.addString (id);

            opPushInt (code, index);

            std::copy (value.begin(), value.end(), std::back_inserter (code));

            if (localType!=valueType)
            {
                if (localType=='f' && (valueType=='l' || valueType=='s'))
                {
                    opIntToFloat (code);
                }
                else if ((localType=='l' || localType=='s') && valueType=='f')
                {
                    opFloatToInt (code);
                }
            }

            switch (localType)
            {
                case 'f':

                    opStoreScriptMemberFloat (code, global);
                    break;

                case 's':

                    opStoreScriptMemberShort (code, global);
                    break;

                case 'l':

                    opStoreScriptMemberLong (code, global);
                    break;

                case 'r':

                    opStoreScriptMemberRef (code, global);
                    break;

                default:

                    assert (0);
            }
        }

        void fetchMember (CodeContainer& code, Compiler::Literals& literals, char localType,
            const std::string& name, const std::string& id, bool global)
        {
            int index = literals.addString (name);

            opPushInt (code, index);

            index = literals.addString (id);

            opPushInt (code, index);

            switch (localType)
            {
                case 'f':

                    opFetchScriptMemberFloat (code, global);
                    break;

                case 's':

                    opFetchScriptMemberShort (code, global);
                    break;

                case 'l':

                    opFetchScriptMemberLong (code, global);
                    break;

                case 'r':

                    opFetchScriptMemberRef (code, global);
                    break;

                default:

                    assert (0);
            }
        }

        void random (CodeContainer& code)
        {
            opRandom (code);
        }

        void scriptRunning (CodeContainer& code)
        {
            opScriptRunning (code);
        }

        void startScript (CodeContainer& code, Compiler::Literals& literals, const std::string& id)
        {
            if (id.empty())
                opStartScript (code, false);
            else
            {
                int index = literals.addString (id);
                opPushInt (code, index);
                opStartScript (code, true);
            }
        }

        void stopScript (CodeContainer& code)
        {
            opStopScript (code);
        }

        void getDistance (CodeContainer& code, Compiler::Literals& literals, const std::string& id)
        {
            if (id.empty())
            {
                opGetDistance (code);
            }
            else
            {
                int index = literals.addString (id);
                opPushInt (code, index);
                opGetDistanceExplicit (code);
            }
        }

        void getSecondsPassed (CodeContainer& code)
        {
            opGetSecondsPassed (code);
        }

        void getDisabled (CodeContainer& code, Compiler::Literals& literals, const std::string& id)
        {
            if (id.empty())
            {
                opGetDisabled (code);
            }
            else
            {
                int index = literals.addString (id);
                opPushInt (code, index);
                opGetDisabledExplicit (code);
            }
        }

        void enable (CodeContainer& code, Compiler::Literals& literals, const std::string& id)
        {
            if (id.empty())
            {
                opEnable (code);
            }
            else
            {
                int index = literals.addString (id);
                opPushInt (code, index);
                opEnableExplicit (code);
            }
        }

        void disable (CodeContainer& code, Compiler::Literals& literals, const std::string& id)
        {
            if (id.empty())
            {
                opDisable (code);
            }
            else
            {
                int index = literals.addString (id);
                opPushInt (code, index);
                opDisableExplicit (code);
            }
        }
    }
}
