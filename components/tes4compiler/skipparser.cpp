#include "skipparser.hpp"

#include "scanner.hpp"

namespace Tes4Compiler
{
    SkipParser::SkipParser (Compiler::ErrorHandler& errorHandler, const Compiler::Context& context)
    : Parser (errorHandler, context)
    {}

    bool SkipParser::parseInt (int value, const Compiler::TokenLoc& loc, Scanner& scanner)
    {
        return true;
    }

    bool SkipParser::parseFloat (float value, const Compiler::TokenLoc& loc, Scanner& scanner)
    {
        return true;
    }

    bool SkipParser::parseName (const std::string& name, const Compiler::TokenLoc& loc,
        Scanner& scanner)
    {
        return true;
    }

    bool SkipParser::parseKeyword (int keyword, const Compiler::TokenLoc& loc, Scanner& scanner)
    {
        return true;
    }

    bool SkipParser::parseSpecial (int code, const Compiler::TokenLoc& loc, Scanner& scanner)
    {
        if (code==Scanner::S_newline)
            return false;

        return true;
    }

    void SkipParser::parseEOF(Scanner& scanner)
    {

    }
}

