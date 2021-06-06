#ifndef PRINT_DEBUG_H
#define PRINT_DEBUG_H

#include <sstream>
#include <cc.h>
#include <compilation/instruction.h>

namespace cc
{
    /* AST Debug */
    std::ostream& p(std::ostream &ss, const Expression* self);
    std::ostream& p(std::ostream &ss, const Statement* self, int indent = 0);
    std::ostream &p(std::ostream &ss, const ASTFunctionDefine* self);
    std::ostream &p(std::ostream &ss, const ASTGlobalVariable* self);
    std::ostream& p(std::ostream& ss, const ASTGlobal* self);

    /* IR Debug */
    std::ostream& p(std::ostream &ss, const Reference* self);
    std::ostream& p(std::ostream &ss, const Instruction* self);
    std::ostream& p(std::ostream &ss, const Constant* self);
    std::ostream& p(std::ostream& ss, const Block* self);
    std::ostream& p(std::ostream& ss, const Scope* self);
}

#endif //PRINT_DEBUG_H
