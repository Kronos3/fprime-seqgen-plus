#include <map>
#include "print_debug.h"
#include <compilation/type.h>

using namespace cc;

#define INDENT_STR "    "

namespace cc
{
    void print_indent(std::stringstream &ss, int indent);

    void print_for(std::stringstream &ss, const ForLoop* t, int indent);

    void print_while(std::stringstream &ss, const WhileLoop* t, int indent);

    void print_call_expr(std::stringstream &ss, const CallExpr* self);

    void print_bin_expr(std::stringstream &ss, const BinaryExpr* self);

    void print_unary_expr(std::stringstream &ss, const UnaryExpr* self);

    void print_stmt_decl_init(std::stringstream &ss, const DeclInit* self, int indent);

    void print_stmt_decl(std::stringstream &ss, const Decl* self, int indent);

    void print_stmt_eval(std::stringstream &ss, const Eval* self, int indent);

    void print_stmt_multi(std::stringstream &ss, const MultiStatement* self, int indent);

    void print_stmt_if(std::stringstream &ss, const If* self, int indent);

    std::stringstream &p(std::stringstream &ss,
                         const Statement* self,
                         int indent)
    {
        if (dynamic_cast<const DeclInit*>(self))
        {
            print_stmt_decl_init(ss, dynamic_cast<const DeclInit*>(self), indent);
        }
        else if (dynamic_cast<const Decl*>(self))
        {
            print_stmt_decl(ss, dynamic_cast<const Decl*>(self), indent);
        }
        else if (dynamic_cast<const Eval*>(self))
        {
            print_stmt_eval(ss, dynamic_cast<const Eval*>(self), indent);
        }
        else if (dynamic_cast<const ForLoop*>(self))
        {
            print_for(ss, dynamic_cast<const ForLoop*>(self), indent);
        }
        else if (dynamic_cast<const WhileLoop*>(self))
        {
            print_while(ss, dynamic_cast<const WhileLoop*>(self), indent);
        }
        else if (dynamic_cast<const If*>(self))
        {
            print_stmt_if(ss, dynamic_cast<const If*>(self), indent);
        }
        else if (dynamic_cast<const MultiStatement*>(self))
        {
            print_stmt_multi(ss, dynamic_cast<const MultiStatement*>(self), indent);
        }
        else if (dynamic_cast<const Continue*>(self))
        {
            print_indent(ss, indent);
            ss << "Continue";
        }
        else if (dynamic_cast<const Break*>(self))
        {
            print_indent(ss, indent);
            ss << "Break";
        }
        else if (dynamic_cast<const Return*>(self))
        {
            print_indent(ss, indent);
            ss << "Return";
        }
        else
        {
            throw Exception("Invalid Statement");
        }

        return ss;
    }

    std::stringstream &p(std::stringstream &ss, const Expression* self)
    {
        if (dynamic_cast<const NumericExpr*>(self))
        {
            const auto* self_c = dynamic_cast<const NumericExpr*>(self);
            switch(self_c->type)
            {
                case NumericExpr::INTEGER:
                    ss << "Int(" << self_c->value.integer << ")";
                    break;
                case NumericExpr::FLOATING:
                    ss << "Float(" << self_c->value.floating << ")";
                    break;
            }
        }
        else if (dynamic_cast<const VariableExpr*>(self))
        {
            ss << "Var(" << dynamic_cast<const VariableExpr*>(self)->variable << ")";
        }
        else if (dynamic_cast<const LiteralExpr*>(self))
        {
            ss << "Literal(\"" << dynamic_cast<const LiteralExpr*>(self)->value << "\")";
        }
        else if (dynamic_cast<const AssignExpr*>(self))
        {
            ss << "Assign(";
            p(ss, dynamic_cast<const AssignExpr*>(self)->sink) << " = ";
            p(ss, dynamic_cast<const AssignExpr*>(self)->value) << ")";
        }
        else if (dynamic_cast<const CallExpr*>(self))
        {
            print_call_expr(ss, dynamic_cast<const CallExpr*>(self));
        }
        else if (dynamic_cast<const BinaryExpr*>(self))
        {
            print_bin_expr(ss, dynamic_cast<const BinaryExpr*>(self));
        }
        else if (dynamic_cast<const UnaryExpr*>(self))
        {
            print_unary_expr(ss, dynamic_cast<const UnaryExpr*>(self));
        }
        else
        {
            throw Exception("Invalid Expression");
        }

        return ss;
    }

    std::stringstream &p(std::stringstream &ss, const ASTFunctionDefine* self)
    {
        ss << self->return_type->as_string() << " " << self->name << "(";
        for (Arguments* iter = self->args; iter; iter = iter->next)
        {
            ss << iter->decl->type->as_string() << " " << iter->decl->name;
            if (iter->next)
            {
                ss << ", ";
            }
        }
        ss << ")\n";
        p(ss, self->body, 1);
        return ss;
    }

    std::stringstream &p(std::stringstream &ss, const ASTGlobalVariable* self)
    {
        ss << self->decl->type->as_string() << " " << self->decl->name << "(";

        return ss;
    }

    std::stringstream &p(std::stringstream &ss, const ASTGlobal* self)
    {
        for (const ASTGlobal* iter = self; iter; iter = iter->next)
        {
            if (dynamic_cast<const ASTFunctionDefine*>(iter))
            {
                p(ss, dynamic_cast<const ASTFunctionDefine*>(iter)) << "\n";
            }
            else if (dynamic_cast<const ASTGlobalVariable*>(iter))
            {
                p(ss, dynamic_cast<const ASTGlobalVariable*>(iter)) << "\n";
            }
        }
        return ss;
    }

    void print_indent(std::stringstream &ss, int indent)
    {
        for (int i = 0; i < indent; i++)
        {
            ss << INDENT_STR;
        }
    }

    void print_for(std::stringstream &ss, const ForLoop* t, int indent)
    {
        print_indent(ss, indent);
        ss << "For(";
        p(ss, t->initial, 0) << "; ";
        p(ss, t->conditional) << "; ";
        p(ss, t->increment) << ")\n";
        p(ss, t->body, indent + 1);
    }

    void print_while(std::stringstream &ss, const WhileLoop* t, int indent)
    {
        print_indent(ss, indent);
        ss << "While(";
        p(ss, t->conditional) << ")\n";
        p(ss, t->body, indent + 1);
    }

    void print_stmt_decl_init(std::stringstream &ss, const DeclInit* self, int indent)
    {
        print_indent(ss, indent);
        ss << "DeclInit(["
           << self->decl->type->as_string() << "] "
           << self->decl->name << " = ";
        p(ss, self->initializer) << ")";
    }

    void print_stmt_decl(std::stringstream &ss, const Decl* self, int indent)
    {
        print_indent(ss, indent);
        ss << "Decl(["
           << self->decl->type->as_string() << "] "
           << self->decl->name << ")";
    }

    void print_stmt_eval(std::stringstream &ss, const Eval* self, int indent)
    {
        print_indent(ss, indent);
        p(ss, self->expr);
    }

    void print_stmt_multi(std::stringstream &ss, const MultiStatement* self, int indent)
    {
        print_indent(ss, indent - 1);
        ss << "{\n";

        for (const MultiStatement* iter = self; iter; iter = iter->next)
        {
            p(ss, iter->self, indent) << "\n";
        }
        print_indent(ss, indent - 1);
        ss << "}";
    }

    void print_stmt_if(std::stringstream &ss, const If* self, int indent)
    {
        print_indent(ss, indent);
        ss << "If(";
        p(ss, self->clause) << ")\n";

        if (self->then_stmt)
        {
            p(ss, self->then_stmt, indent + 1);
        }
        else
        {
            ss << "{}";
        }

        ss << "\n";
        if (self->else_stmt)
        {
            print_indent(ss, indent);
            ss << "else\n";
            p(ss, self->else_stmt, indent + 1);
        }
    }

    void print_call_expr(std::stringstream &ss, const CallExpr* self)
    {
        ss << "CallExpr(" << self->function << " args=[";
        for (const CallArguments* iter = self->arguments; iter; iter = iter->next)
        {
            p(ss, iter->value);
            if (iter->next)
            {
                ss << ", ";
            }
        }
        ss << (self->arguments ? "" : "void") << "])";
    }

    void print_bin_expr(std::stringstream &ss, const BinaryExpr* self)
    {
        std::map<BinaryExpr::binary_operator_t, std::string> op_map{
                {BinaryExpr::A_ADD, "+"},
                {BinaryExpr::A_SUB, "-"},
                {BinaryExpr::A_DIV, "/"},
                {BinaryExpr::A_MUL, "*"},
                {BinaryExpr::B_AND, "&"},
                {BinaryExpr::B_OR,  "|"},
                {BinaryExpr::B_XOR, "^"},
                {BinaryExpr::L_LT,  "<"},
                {BinaryExpr::L_GT,  ">"},
                {BinaryExpr::L_LE,  "<="},
                {BinaryExpr::L_GE,  ">="},
                {BinaryExpr::L_EQ,  "=="},
                {BinaryExpr::L_AND, "&&"},
                {BinaryExpr::L_OR,  "||"},
        };

        ss << "BinExpr(";
        p(ss, self->a) << " " << op_map[self->op] << " ";
        p(ss, self->b) << ")";
    }

    void print_unary_expr(std::stringstream &ss, const UnaryExpr* self)
    {
        std::map<UnaryExpr::unary_operator_t, std::string> op_map{
                {UnaryExpr::B_NOT,    "~"},
                {UnaryExpr::L_NOT,    "!"},
                {UnaryExpr::INC_PRE,  "INC_PRE "},
                {UnaryExpr::INC_POST, "INC_POST "},
                {UnaryExpr::DEC_PRE,  "DEC_PRE "},
                {UnaryExpr::DEC_POST, "DEC_POST "},
        };

        ss << "UnaryExpr(" << op_map[self->op];
        p(ss, self->operand) << ")";
    }
}
