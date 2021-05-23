#ifndef PRINT_DEBUG_H
#define PRINT_DEBUG_H

#include <sstream>
#include <cc.h>
#include <compilation/instruction.h>

namespace cc
{
    /* AST Debug */
    std::stringstream& p(std::stringstream &ss, const Expression* self);
    std::stringstream& p(std::stringstream &ss, const Statement* self, int indent = 0);
    std::stringstream &p(std::stringstream &ss, const ASTFunctionDefine* self);
    std::stringstream &p(std::stringstream &ss, const ASTGlobalVariable* self);
    std::stringstream& p(std::stringstream& ss, const ASTGlobal* self);

    /* IR Debug */
    std::stringstream& p(std::stringstream &ss, const Reference* self);
    std::stringstream& p(std::stringstream &ss, const Instruction* self);
    std::stringstream& p(std::stringstream &ss, const Constant* self);
    std::stringstream& p(std::stringstream& ss, const Block* self);
    std::stringstream& p(std::stringstream& ss, const Scope* self);
}

#endif //PRINT_DEBUG_H
