#include <cc.h>
#include "context.h"

namespace cc
{
#define PERFORM_TRAVERSAL(field) do { \
    (field)->traverse(cb, build_scope, data);    \
} while(0)

    void Loop::traverse(TraverseCB cb, Context* ctx, void* data)
    {
        conditional->traverse(cb, ctx, data);
        body->traverse(cb, ctx, data);
        cb(this, ctx, data);
    }

    void ForLoop::traverse(TraverseCB cb, Context* ctx, void* data)
    {
        initial->traverse(cb, ctx, data);
        conditional->traverse(cb, ctx, data);
        increment->traverse(cb, ctx, data);
        body->traverse(cb, ctx, data);
        cb(this, ctx, data);
    }

    void MultiStatement::traverse(TraverseCB cb, Context* ctx, void* data)
    {
        ctx->enter_scope("block");
        for (MultiStatement* iter = this; iter; iter = iter->next)
        {
            iter->self->traverse(cb, ctx, data);
            cb(iter, ctx, data);
        }
        ctx->exit_scope();
    }

    void Function::traverse(TraverseCB cb, Context* ctx, void* data)
    {
        ctx->enter_scope(name);
        if (args)
        {
            args->traverse(cb, ctx, data);
        }

        body->traverse(cb, ctx, data);
        cb(this, ctx, data);
        ctx->exit_scope();
    }

    void Arguments::traverse(TraverseCB cb, Context* ctx, void* data)
    {
        for (Arguments* iter = this; iter; iter = iter->next)
        {
            iter->decl->traverse(cb, ctx, data);
        }
    }

    void GlobalDeclaration::traverse(TraverseCB cb, Context* ctx, void* data)
    {
        for (GlobalDeclaration* iter = this; iter; iter = iter->next)
        {
            iter->self->traverse(cb, ctx, data);
        }
    }

    void Decl::traverse(TraverseCB cb, Context* ctx, void* data)
    {
        decl->traverse(cb, ctx, data);
        cb(this, ctx, data);
    }

    void DeclInit::traverse(TraverseCB cb, Context* ctx, void* data)
    {
        Decl::traverse(cb, ctx, data);
        initializer->traverse(cb, ctx, data);
    }

    void Eval::traverse(TraverseCB cb, Context* ctx, void* data)
    {
        expr->traverse(cb, ctx, data);
        cb(this, ctx, data);
    }

    void If::traverse(TraverseCB cb, Context* ctx, void* data)
    {
        clause->traverse(cb, ctx, data);
        assert(then_stmt);

        ctx->enter_scope("if");
        then_stmt->traverse(cb, ctx, data);
        ctx->exit_scope();

        if (else_stmt)
        {
            ctx->enter_scope("else");
            else_stmt->traverse(cb, ctx, data);
            ctx->exit_scope();
        }

        cb(this, ctx, data);
    }

    void BinaryExpr::traverse(TraverseCB cb, Context* ctx, void* data)
    {
        a->traverse(cb, ctx, data);
        b->traverse(cb, ctx, data);
        cb(this, ctx, data);
    }

    void UnaryExpr::traverse(TraverseCB cb, Context* ctx, void* data)
    {
        operand->traverse(cb, ctx, data);
        cb(this, ctx, data);
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

        cb(this, ctx, data);
    }

    void AssignExpr::traverse(TraverseCB cb, Context* ctx, void* data)
    {
        sink->traverse(cb, ctx, data);
        value->traverse(cb, ctx, data);
        cb(this, ctx, data);
    }

    static void resolution_pass_cb(ASTValue* self, Context* ctx)
    {
        self->resolution_pass(ctx);
    }

    void perform_resolution(ASTValue* self, Context* ctx)
    {
        ctx->start_scope_build();
        self->traverse(reinterpret_cast<ASTValue::TraverseCB>(resolution_pass_cb), ctx, nullptr);
        ctx->end_scope_build();
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
            throw ASTException(this, "Undeclared variable " + variable);
        }
    }

    void TypeDecl::resolution_pass(Context* context)
    {
        variable = context->declare_variable(this);
    }
}
