#ifndef PRINT_DEBUG_H
#define PRINT_DEBUG_H

#include <sstream>
#include <cc.h>

namespace cc
{
    std::stringstream& p(std::stringstream &ss, const Expression* self);
    std::stringstream& p(std::stringstream &ss, const Statement* self, int indent = 0);
    std::stringstream& p(std::stringstream& ss, const ASTFunction* self);
    std::stringstream& p(std::stringstream& ss, const ASTGlobal* self);
}

#endif //PRINT_DEBUG_H
