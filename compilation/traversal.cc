#include <cc.h>
#include "context.h"
#include "module.h"

namespace cc
{
    void ForLoop::traverse(TraverseCB cb, Context* ctx, void* data)
    {
        ctx->enter_scope(Scope::LOOP);

        initial->traverse(cb, ctx, data);
        conditional->traverse(cb, ctx, data);
        increment->traverse(cb, ctx, data);
        body->traverse(cb, ctx, data);
        ASTValue::traverse(cb, ctx, data);

        ctx->exit_scope();
    }


    void WhileLoop::traverse(TraverseCB cb, Context* ctx, void* data)
    {
        ctx->enter_scope(Scope::LOOP);

        conditional->traverse(cb, ctx, data);
        body->traverse(cb, ctx, data);
        ASTValue::traverse(cb, ctx, data);

        ctx->exit_scope();
    }

    void MultiStatement::traverse(TraverseCB cb, Context* ctx, void* data)
    {
        ctx->enter_scope(Scope::BRACKET);
        for (MultiStatement* iter = this; iter; iter = iter->next)
        {
            iter->self->traverse(cb, ctx, data);
            cb(iter, ctx, data);
        }
        ctx->exit_scope();
    }

    void Arguments::traverse(TraverseCB cb, Context* ctx, void* data)
    {
        for (Arguments* iter = this; iter; iter = iter->next)
        {
            iter->decl->traverse(cb, ctx, data);
        }
    }

    void Decl::traverse(TraverseCB cb, Context* ctx, void* data)
    {
        decl->traverse(cb, ctx, data);
        ASTValue::traverse(cb, ctx, data);
    }

    void DeclInit::traverse(TraverseCB cb, Context* ctx, void* data)
    {
        Decl::traverse(cb, ctx, data);
        initializer->traverse(cb, ctx, data);
    }

    void Eval::traverse(TraverseCB cb, Context* ctx, void* data)
    {
        expr->traverse(cb, ctx, data);
        ASTValue::traverse(cb, ctx, data);
    }

    void If::traverse(TraverseCB cb, Context* ctx, void* data)
    {
        clause->traverse(cb, ctx, data);
        assert(then_stmt);

        then_stmt->traverse(cb, ctx, data);

        if (else_stmt)
        {
            else_stmt->traverse(cb, ctx, data);
        }

        ASTValue::traverse(cb, ctx, data);
    }

    void BinaryExpr::traverse(TraverseCB cb, Context* ctx, void* data)
    {
        a->traverse(cb, ctx, data);
        b->traverse(cb, ctx, data);
        ASTValue::traverse(cb, ctx, data);
    }

    void UnaryExpr::traverse(TraverseCB cb, Context* ctx, void* data)
    {
        operand->traverse(cb, ctx, data);
        ASTValue::traverse(cb, ctx, data);
    }

    void CallArguments::traverse(TraverseCB cb, Context* ctx, void* data)
    {
        for (CallArguments* iter = this; iter; iter = iter->next)
        {
            iter->value->traverse(cb, ctx, data);
        }
    }

    void CallExpr::traverse(TraverseCB cb, Context* ctx, void* data)
    {
        if (arguments)
        {
            arguments->traverse(cb, ctx, data);
        }

        ASTValue::traverse(cb, ctx, data);
    }

    void AssignExpr::traverse(TraverseCB cb, Context* ctx, void* data)
    {
        sink->traverse(cb, ctx, data);
        value->traverse(cb, ctx, data);
        ASTValue::traverse(cb, ctx, data);
    }

    void Return::traverse(ASTValue::TraverseCB cb, Context* ctx, void* data)
    {
        if (return_value)
        {
            return_value->traverse(cb, ctx, data);
        }

        ASTValue::traverse(cb, ctx, data);
    }

    void ASTFunctionDefine::traverse(TraverseCB cb, Context* ctx, void* data)
    {
        ctx->enter_scope(Scope::FUNCTION, name);
        if (args)
        {
            args->traverse(cb, ctx, data);
        }

        body->traverse(cb, ctx, data);
        cc::ASTFunction::traverse(cb, ctx, data);
        ctx->exit_scope();
    }

    void ASTGlobalVariable::traverse(ASTValue::TraverseCB cb, Context* ctx, void* data)
    {
        decl->traverse(cb, ctx, data);
        ASTGlobal::traverse(cb, ctx, data);
    }

    Expression* BinaryExpr::reduce(
            Context* ctx,
            Expression* a, Expression* b,
            BinaryExpr::binary_operator_t op)
    {
        auto* out = new BinaryExpr(a, b, op);
        if (dynamic_cast<ASTConstant*>(a)
            && dynamic_cast<ASTConstant*>(b))
        {
            auto* c = out->get_constant(ctx);
            auto* c_e = dynamic_cast<ASTConstant*>(c);
            if (not c_e)
            {
                delete c;
            }
            else
            {
                delete out;
                return c_e;
            }
        }

        return out;
    }

    Expression* UnaryExpr::reduce(
            Context* ctx, Expression* operand, unary_operator_t op)
    {
        auto* out = new UnaryExpr(operand, op);
        if (dynamic_cast<ASTConstant*>(operand))
        {
            auto* c = out->get_constant(ctx);
            auto* c_e = dynamic_cast<ASTConstant*>(c);
            if (not c_e)
            {
                delete c;
            }
            else
            {
                delete out;
                return c_e;
            }
        }

        return out;
    }

    /************************************************************************
     *
     *                          Resolution Pass                             *
     *
     * *********************************************************************/

    void VariableExpr::resolution_pass(Context* context)
    {
        value = context->get_variable(variable);
        if (!value)
        {
            context->emit_error(this, "Undeclared variable " + variable);
        }
    }

    void TypeDecl::resolution_pass(Context* context)
    {
        variable = context->declare_variable(this);
        if (!variable)
        {
            context->emit_error(this, "Redeclared variable '" + this->name + "'");
        }
    }

    void ASTGlobalVariable::resolution_pass(Context* context)
    {
        symbol = context->get_module()->declare_variable(this);
    }

    void ASTFunction::resolution_pass(Context* context)
    {
        symbol = context->get_module()->declare_function(this);
    }
}
