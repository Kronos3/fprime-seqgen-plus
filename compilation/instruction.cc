
#include <cc.h>
#include <cstring>

#include "instruction.h"
#include "context.h"
#include "module.h"

namespace cc
{
    void ForLoop::add(Context* ctx, IRBuilder &IRB) const
    {
        Block* parent_block = IRB.get_insertion_point();
        Scope* parent_scope = ctx->scope();
        ctx->enter_scope(Scope::LOOP);
        auto* loop_scope = dynamic_cast<LoopScope*>(ctx->scope());
        assert(loop_scope);

        // Add the initial instructions to the parent block
        // we don't want to execute these multiple times
        initial->add(ctx, IRB);

        // Preemptively create a block that will be jumped
        // to on break or condition failure
        Block* post_block = parent_scope->new_block();
        Block* loop_block = loop_scope->get_entry_block();

        // Set up the for loop execution order
        parent_block->chain(loop_block);
        loop_scope->set_exit(post_block);

        // Add the conditional instructions
        IRB.set_insertion_point(loop_block);
        IRB.add<BranchInstr>(post_block, IRB.add<L_NotInstr>(conditional->get(ctx, IRB)));

        // Insert the looping instructions into the body block
        body->add(ctx, IRB);

        // Add the iteration instructions
        increment->get(ctx, IRB);
        IRB.add<JumpInstr>(loop_block);

        // Get the context read for the next instruction
        IRB.set_insertion_point(post_block);
        ctx->exit_scope();
    }

    void WhileLoop::add(Context* ctx, IRBuilder &IRB) const
    {
        Block* parent_block = IRB.get_insertion_point();
        Scope* parent_scope = ctx->scope();
        ctx->enter_scope(Scope::LOOP);
        auto* loop_scope = dynamic_cast<LoopScope*>(ctx->scope());
        assert(loop_scope);

        // Preemptively create a block that will be jumped
        // to on break or condition failure
        Block* post_block = parent_scope->new_block();
        Block* loop_block = loop_scope->get_entry_block();

        // Set up the for loop execution order
        parent_block->chain(loop_block);
        loop_scope->set_exit(post_block);

        // Add the conditional instructions
        IRB.set_insertion_point(loop_block);
        IRB.add<BranchInstr>(post_block, new L_NotInstr(conditional->get(ctx, IRB)));

        // Loop
        IRB.add<JumpInstr>(loop_block);

        // Get the context read for the next instruction
        IRB.set_insertion_point(post_block);
        ctx->exit_scope();
    }

    void MultiStatement::add(Context* ctx, IRBuilder &IRB) const
    {
        ctx->enter_scope(Scope::BRACKET);
        for (const MultiStatement* iter = this; iter; iter = iter->next)
        {
            iter->self->add(ctx, IRB);
        }
        ctx->exit_scope();
    }

    void Decl::add(Context* ctx, IRBuilder &IRB) const
    {
        decl->variable->set(IRB.add<AllocaInstr>(decl->type));
    }

    void DeclInit::add(Context* ctx, IRBuilder &IRB) const
    {
        Decl::add(ctx, IRB);
        IRB.add<MovInstr>(decl->variable->get(), initializer->get(ctx, IRB));
    }

    void Eval::add(Context* ctx, IRBuilder &IRB) const
    {
        // Discard the output
        expr->get(ctx, IRB);
    }

    void If::add(Context* ctx, IRBuilder &IRB) const
    {
        Block* then_block = ctx->scope()->new_block();
        IRB.add<BranchInstr>(then_block, clause->get(ctx, IRB));

        Block* post_block = ctx->scope()->new_block();
        then_block->chain(post_block);

        if (else_stmt)
        {
            Block* else_block = ctx->scope()->new_block();
            else_block->chain(post_block);
            IRB.add<JumpInstr>(else_block, true);

            IRB.set_insertion_point(else_block);
            else_stmt->add(ctx, IRB);
        }

        IRB.set_insertion_point(then_block);
        then_stmt->add(ctx, IRB);

        IRB.set_insertion_point(post_block);
    }

    void Continue::add(Context* ctx, IRBuilder &IRB) const
    {
        LoopScope* parent_loop = ctx->scope()->get_loop();
        if (!parent_loop)
        {
            ctx->emit_error(this, "continue must be inside a loop");
            return;
        }

        IRB.add<JumpInstr>(parent_loop->get_entry_block());
    }

    void Break::add(Context* ctx, IRBuilder &IRB) const
    {
        LoopScope* parent_loop = ctx->scope()->get_loop();
        if (!parent_loop)
        {
            ctx->emit_error(this, "break must be inside a loop");
            return;
        }

        IRB.add<JumpInstr>(parent_loop->get_exit());
    }

    void Return::add(Context* ctx, IRBuilder &IRB) const
    {
        IRB.add<ReturnInstr>();
    }

    const IR* BinaryExpr::get(Context* ctx, IRBuilder &IRB) const
    {
        switch (op)
        {
            case A_ADD: return IRB.add<AddInstr>(a->get(ctx, IRB), b->get(ctx, IRB));
            case A_SUB: return IRB.add<SubInstr>(a->get(ctx, IRB), b->get(ctx, IRB));
            case A_DIV: return IRB.add<DivInstr>(a->get(ctx, IRB), b->get(ctx, IRB));
            case A_MUL: return IRB.add<MulInstr>(a->get(ctx, IRB), b->get(ctx, IRB));
            case B_AND: return IRB.add<B_AndInstr>(a->get(ctx, IRB), b->get(ctx, IRB));
            case B_OR: return IRB.add<B_OrInstr>(a->get(ctx, IRB), b->get(ctx, IRB));
            case B_XOR: return IRB.add<B_XorInstr>(a->get(ctx, IRB), b->get(ctx, IRB));
            case L_LT: return IRB.add<LTInstr>(a->get(ctx, IRB), b->get(ctx, IRB));
            case L_GT: return IRB.add<GTInstr>(a->get(ctx, IRB), b->get(ctx, IRB));
            case L_LE: return IRB.add<LEInstr>(a->get(ctx, IRB), b->get(ctx, IRB));
            case L_GE: return IRB.add<GEInstr>(a->get(ctx, IRB), b->get(ctx, IRB));
            case L_EQ: return IRB.add<EQInstr>(a->get(ctx, IRB), b->get(ctx, IRB));
            case L_AND: return IRB.add<L_AndInstr>(a->get(ctx, IRB), b->get(ctx, IRB));
            case L_OR: return IRB.add<L_OrInstr>(a->get(ctx, IRB), b->get(ctx, IRB));
        }

        ctx->emit_error(a, "Invalid Binary expression: " + std::to_string(op));
        return nullptr;
    }

    const IR* UnaryExpr::get(Context* ctx, IRBuilder &IRB) const
    {
        switch (op)
        {
            case B_NOT: return IRB.add<B_NotInstr>(operand->get(ctx, IRB));
            case L_NOT: return IRB.add<L_NotInstr>(operand->get(ctx, IRB));
            case INC_PRE: return IRB.add<IncInstr>(operand->get(ctx, IRB));
            case DEC_PRE: return IRB.add<DecInstr>(operand->get(ctx, IRB));
            case INC_POST:
            {
                const IR* out = operand->get(ctx, IRB);
                IRB.add<IncInstr>(out);
                return out;
            }
            case DEC_POST:
            {
                const IR* out = operand->get(ctx, IRB);
                IRB.add<DecInstr>(out);
                return out;
            }
        }

        ctx->emit_error(operand, "Invalid Unary expression: " + std::to_string(op));
        return nullptr;
    }

    const IR* ASTConstant::get(Context* ctx, IRBuilder &IRB) const
    {
        return this;
    }

    std::string ImmIntExpr::get_name() const
    {
        static int i = 0;
        return variadic_string("integer.%d", i++);
    }

    std::string ImmFloatExpr::get_name() const
    {
        static int i = 0;
        return variadic_string("float.%d", i++);
    }

    std::string LiteralExpr::get_name() const
    {
        static int i = 0;
        return variadic_string("str.%d", i++);
    }

    const IR* VariableExpr::get(Context* ctx, IRBuilder &IRB) const
    {
        return value->get();
    }

    void LiteralExpr::write(void* buffer) const
    {
        memcpy(buffer, value.c_str(), value.length() + 1);
    }

    const IR* CallExpr::get(Context* ctx, IRBuilder &IRB) const
    {
        /* Resolve the function declaration */
        const Function* F = ctx->get_module()->get_function(function);
        if (!F)
        {
            ctx->emit_error(this, "Undeclared function: " + function);
        }

        std::vector<const IR*> args_ir;
        for (const CallArguments* arg = arguments; arg; arg = arg->next)
        {
            args_ir.push_back(arg->value->get(ctx, IRB));
        }

        return IRB.add<CallInstr>(F, args_ir);
    }

    const IR* AssignExpr::get(Context* ctx, IRBuilder &IRB) const
    {
        const IR* out = value->get(ctx, IRB);
        const IR* sink_val = sink->get(ctx, IRB);
        auto* ref = dynamic_cast<const Reference*>(sink_val);
        if (!ref)
        {
            ctx->emit_error(sink, "Expression does not return a reference");
            return nullptr;
        }

        IRB.add<MovInstr>(ref, out);
        return out;
    }

    void ASTFunctionDefine::add(Context* ctx, IRBuilder &IRB) const
    {
        ctx->enter_scope(Scope::FUNCTION, name);
        auto* f = dynamic_cast<Function*>(symbol);
        assert(f);

        f->set_entry_block(ctx->scope()->get_entry_block());
        IRB.set_insertion_point(f->get_entry_block());

        /* Declare the arguments */
        for (Arguments* iter = args; iter; iter = iter->next)
        {
            iter->decl->variable->set(IRB.add<AllocaInstr>(iter->decl->type));
        }

        /* Add the function code */
        body->add(ctx, IRB);

        /* Clean up */
        IRB.set_insertion_point(nullptr);
        ctx->exit_scope();
    }

    void ASTGlobalVariable::add(Context* ctx, IRBuilder &IRB) const
    {
        auto* gv = dynamic_cast<GlobalVariable*>(symbol);
        assert(gv);
        decl->variable->set(gv);

        if (initializer)
        {
            IRB.set_insertion_point(ctx->get_module()->constructor());
            IRB.add<MovInstr>(gv, initializer->get(ctx, IRB));
        }
    }

    Block::~Block()
    {
        for (Instruction* instr : instructions)
        {
            delete instr;
        }
        instructions.clear();

        for (IR* ir : dangling)
        {
            delete ir;
        }
        dangling.clear();
    }
}
