#include <map>
#include "print_debug.h"
#include <compilation/type.h>

using namespace cc;

#define INDENT_STR "    "

namespace cc
{
    void print_indent(std::ostream &ss, int indent);

    void print_for(std::ostream &ss, const ForLoop* t, int indent);

    void print_while(std::ostream &ss, const WhileLoop* t, int indent);

    void print_call_expr(std::ostream &ss, const CallExpr* self);

    void print_bin_expr(std::ostream &ss, const BinaryExpr* self);

    void print_unary_expr(std::ostream &ss, const UnaryExpr* self);

    void print_stmt_decl_init(std::ostream &ss, const DeclInit* self, int indent);

    void print_stmt_decl(std::ostream &ss, const Decl* self, int indent);

    void print_stmt_eval(std::ostream &ss, const Eval* self, int indent);

    void print_stmt_multi(std::ostream &ss, const MultiStatement* self, int indent);

    void print_stmt_if(std::ostream &ss, const If* self, int indent);

    std::ostream &p(std::ostream &ss,
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

    std::ostream &p(std::ostream &ss, const Expression* self)
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
        else if (dynamic_cast<const ConstantExpr*>(self))
        {
            p(ss, dynamic_cast<const ConstantExpr*>(self)->constant);
        }
        else
        {
            throw Exception("Invalid Expression");
        }

        return ss;
    }

    std::ostream &p(std::ostream &ss, const ASTFunctionDefine* self)
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

    std::ostream &p(std::ostream &ss, const ASTGlobalVariable* self)
    {
        ss << self->decl->type->as_string() << " " << self->decl->name << "(";

        return ss;
    }

    std::ostream &p(std::ostream &ss, const ASTGlobal* self)
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

    void print_indent(std::ostream &ss, int indent)
    {
        for (int i = 0; i < indent; i++)
        {
            ss << INDENT_STR;
        }
    }

    void print_for(std::ostream &ss, const ForLoop* t, int indent)
    {
        print_indent(ss, indent);
        ss << "For(";
        p(ss, t->initial, 0) << "; ";
        p(ss, t->conditional) << "; ";
        p(ss, t->increment) << ")\n";
        p(ss, t->body, indent + 1);
    }

    void print_while(std::ostream &ss, const WhileLoop* t, int indent)
    {
        print_indent(ss, indent);
        ss << "While(";
        p(ss, t->conditional) << ")\n";
        p(ss, t->body, indent + 1);
    }

    void print_stmt_decl_init(std::ostream &ss, const DeclInit* self, int indent)
    {
        print_indent(ss, indent);
        ss << "DeclInit(["
           << self->decl->type->as_string() << "] "
           << self->decl->name << " = ";
        p(ss, self->initializer) << ")";
    }

    void print_stmt_decl(std::ostream &ss, const Decl* self, int indent)
    {
        print_indent(ss, indent);
        ss << "Decl(["
           << self->decl->type->as_string() << "] "
           << self->decl->name << ")";
    }

    void print_stmt_eval(std::ostream &ss, const Eval* self, int indent)
    {
        print_indent(ss, indent);
        p(ss, self->expr);
    }

    void print_stmt_multi(std::ostream &ss, const MultiStatement* self, int indent)
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

    void print_stmt_if(std::ostream &ss, const If* self, int indent)
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

    void print_call_expr(std::ostream &ss, const CallExpr* self)
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

    void print_bin_expr(std::ostream &ss, const BinaryExpr* self)
    {
        ss << "BinExpr(";
        p(ss, self->a);

        switch(self->op)
        {

            case BinaryExpr::A_ADD:
                ss << " + ";
                break;
            case BinaryExpr::A_SUB:
                ss << " - ";
                break;
            case BinaryExpr::A_DIV:
                ss << " / ";
                break;
            case BinaryExpr::A_MUL:
                ss << " * ";
                break;
            case BinaryExpr::B_AND:
                ss << " & ";
                break;
            case BinaryExpr::B_OR:
                ss << " | ";
                break;
            case BinaryExpr::B_XOR:
                ss << " ^ ";
                break;
            case BinaryExpr::S_LEFT:
                ss << " << ";
                break;
            case BinaryExpr::S_RIGHT:
                ss << " >> ";
                break;
            case BinaryExpr::L_LT:
                ss << " < ";
                break;
            case BinaryExpr::L_GT:
                ss << " > ";
                break;
            case BinaryExpr::L_LE:
                ss << " <= ";
                break;
            case BinaryExpr::L_GE:
                ss << " >= ";
                break;
            case BinaryExpr::L_EQ:
                ss << " == ";
                break;
            case BinaryExpr::L_AND:
                ss << " && ";
                break;
            case BinaryExpr::L_OR:
                ss << " || ";
                break;
            default:
                throw Exception("Invalid BinaryExpr operator");
        }

        p(ss, self->b) << ")";
    }

    void print_unary_expr(std::ostream &ss, const UnaryExpr* self)
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
