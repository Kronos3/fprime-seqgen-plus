
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
        IRB.set_insertion_point(parent_block);
        initial->add(ctx, IRB);

        // Preemptively create a block that will be jumped
        // to on break or condition failure
        Block* post_block = parent_scope->new_block();

        // Create the block to loop over
        Block* loop_block = loop_scope->new_block("loop");

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

        // Create the block to loop over
        Block* loop_block = loop_scope->new_block("loop");

        // Set up the for loop execution order
        parent_block->chain(loop_block);
        loop_scope->set_exit(post_block);

        // Add the conditional instructions
        IRB.set_insertion_point(loop_block);
        IRB.add<BranchInstr>(post_block, IRB.add<L_NotInstr>(conditional->get(ctx, IRB)));

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
        assert(decl->variable);
        decl->variable->set(IRB.add<AllocaInstr>(decl->variable));
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
        Block* parent_block = IRB.get_insertion_point();
        Block* then_block = ctx->scope()->new_block("then");
        IRB.add<BranchInstr>(then_block, clause->get(ctx, IRB));
        Block* curr_exit = ctx->scope()->get_exit();

        Block* post_block;
        if (curr_exit)
        {
            post_block = curr_exit;
        }
        else
        {
            post_block = ctx->scope()->new_block();
            ctx->scope()->set_exit(post_block);
        }

        then_block->chain(post_block);

        if (else_stmt)
        {
            Block* else_block = ctx->scope()->new_block("else");
            else_block->chain(post_block);
            parent_block->chain(else_block, true);

            IRB.set_insertion_point(else_block);
            else_stmt->add(ctx, IRB);
        }
        else
        {
            parent_block->chain(post_block, true);
        }

        IRB.set_insertion_point(then_block);
        then_stmt->add(ctx, IRB);

        ctx->scope()->set_exit(curr_exit);
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
        ctx->get_function()->add_destructor_block(IRB.get_insertion_point());

        if (not return_value)
        {
            if (ctx->get_function()->get_return_type() != ctx->type<Type::VOID>())
            {
                ctx->emit_error(this, "Non-void function requires a return expression");
            }
        }

        // TODO(tumbar) Check output type
        IRB.add<ReturnInstr>(return_value ? return_value->get(ctx, IRB) : nullptr);
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
            case S_LEFT: IRB.add<L_SLInstr>(a->get(ctx, IRB), b->get(ctx, IRB));
            case S_RIGHT:
            {
                const IR* a_ir = a->get(ctx, IRB);
                const IR* b_ir = b->get(ctx, IRB);
                const auto* qual_type = dynamic_cast<const QualType*>(a_ir->get_type(ctx));
                if (qual_type && qual_type->is_unsigned())
                {
                    return IRB.add<L_SRInstr>(a_ir, b_ir);
                }
                else
                {
                    return IRB.add<A_SRInstr>(a_ir, b_ir);
                }
            }
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

    std::string ConstantExpr::get_name() const
    {
        return variadic_string("imm.%p", this);
    }

    const IR* VariableExpr::get(Context* ctx, IRBuilder &IRB) const
    {
        return value->get();
    }

    Constant* UnaryExpr::get_constant(Context* ctx) const
    {
        std::shared_ptr<Constant> eval(operand->get_constant(ctx));
        if (!eval)
        {
            return nullptr;
        }

        switch (op)
        {
            case B_NOT: return ~*eval;
            case L_NOT: return !*eval;
            default:
                throw ASTException(this, "Illegal constant expression");
        }
    }

    Constant* BinaryExpr::get_constant(Context* ctx) const
    {
        std::shared_ptr<Constant> c_a(a->get_constant(ctx));
        std::shared_ptr<Constant> c_b(b->get_constant(ctx));

        if (!c_a || !c_b)
        {
            return nullptr;
        }

        switch (op)
        {
            case A_ADD: return *c_a + &*c_b;
            case A_SUB: return *c_a - &*c_b;
            case A_DIV: return *c_a / &*c_b;
            case A_MUL: return *c_a * &*c_b;
            case B_AND: return *c_a & &*c_b;
            case B_OR: return *c_a | &*c_b;
            case B_XOR: return *c_a ^ &*c_b;
            case L_LT: return *c_a < &*c_b;
            case L_GT: return *c_a > &*c_b;
            case L_LE: return *c_a <= &*c_b;
            case L_GE: return *c_a >= &*c_b;
            case L_EQ: return *c_a == &*c_b;
            case L_AND: return *c_a && &*c_b;
            case L_OR: return *c_a || &*c_b;
            case S_LEFT: return *c_a << &*c_b;
            case S_RIGHT: return *c_a >> &*c_b;
        }

        throw ASTException(this, "Invalid binary operator");
    }

#define BINARY_OPERATOR_IMPL_BOTH(name, op) \
    Constant* name::operator op(const Constant* c) const \
    { \
        const auto* n = dynamic_cast<const NumericExpr*>(c); \
        if (!n) \
        { \
            throw ASTException(this, "Invalid operator with non-numeric constant"); \
        } \
        if (type == FLOATING || n->type == FLOATING) \
        { \
            return new NumericExpr( \
                    this, FLOATING, \
                    (type == FLOATING ? value.floating : static_cast<double>(value.integer)) \
                    op \
                    (n->type == FLOATING ? n->value.floating : static_cast<double>(n->value.integer))); \
        } \
        return new name(this, INTEGER, value.integer op n->value.integer); \
    }

#define BINARY_OPERATOR_IMPL_INT(name, op) \
    Constant* name::operator op(const Constant* c) const \
    { \
        const auto* n = dynamic_cast<const NumericExpr*>(c); \
        if (!n) \
        { \
            throw ASTException(this, "Invalid operator with non-numeric constant"); \
        } \
        if (type == FLOATING || n->type == FLOATING) \
        { \
            throw ASTException(this, "Illegal non-integer constant"); \
        } \
        return new name(this, INTEGER, value.integer op n->value.integer); \
    }

#define BINARY_OPERATOR_ILLEGAL(name, op) \
    Constant* name::operator op(const Constant* c) const { \
        throw ASTException(this, "Illegal constant binary operator with " #name); \
    }

#define BINARY_OPERATOR_CONST_RET(name, op, expr) \
    Constant* name::operator op(const Constant* c) const { \
        expr; \
    }

#define UNARY_OPERATOR_ILLEGAL(name, op) \
    Constant* name::operator op() const { \
        throw ASTException(this, "Illegal constant unary operator with " #name); \
    }

#define UNARY_OPERATOR_CONST_RET(name, op, expr) \
    Constant* name::operator op() const { \
        expr; \
    }

    BINARY_OPERATOR_IMPL_BOTH(NumericExpr, +)
    BINARY_OPERATOR_IMPL_BOTH(NumericExpr, -)
    BINARY_OPERATOR_IMPL_BOTH(NumericExpr, /)
    BINARY_OPERATOR_IMPL_BOTH(NumericExpr, *)
    BINARY_OPERATOR_IMPL_BOTH(NumericExpr, <)
    BINARY_OPERATOR_IMPL_BOTH(NumericExpr, >)
    BINARY_OPERATOR_IMPL_BOTH(NumericExpr, <=)
    BINARY_OPERATOR_IMPL_BOTH(NumericExpr, >=)
    BINARY_OPERATOR_IMPL_BOTH(NumericExpr, ==)
    BINARY_OPERATOR_IMPL_BOTH(NumericExpr, !=)
    BINARY_OPERATOR_IMPL_BOTH(NumericExpr, &&)
    BINARY_OPERATOR_IMPL_BOTH(NumericExpr, ||)
    BINARY_OPERATOR_IMPL_INT(NumericExpr, &)
    BINARY_OPERATOR_IMPL_INT(NumericExpr, |)
    BINARY_OPERATOR_IMPL_INT(NumericExpr, ^)
    BINARY_OPERATOR_IMPL_INT(NumericExpr, <<)
    BINARY_OPERATOR_IMPL_INT(NumericExpr, >>)

    BINARY_OPERATOR_ILLEGAL(LiteralExpr, +)
    BINARY_OPERATOR_ILLEGAL(LiteralExpr, -)
    BINARY_OPERATOR_ILLEGAL(LiteralExpr, /)
    BINARY_OPERATOR_ILLEGAL(LiteralExpr, *)
    BINARY_OPERATOR_ILLEGAL(LiteralExpr, <)
    BINARY_OPERATOR_ILLEGAL(LiteralExpr, >)
    BINARY_OPERATOR_ILLEGAL(LiteralExpr, <=)
    BINARY_OPERATOR_ILLEGAL(LiteralExpr, >=)
    BINARY_OPERATOR_ILLEGAL(LiteralExpr, &)
    BINARY_OPERATOR_ILLEGAL(LiteralExpr, |)
    BINARY_OPERATOR_ILLEGAL(LiteralExpr, ^)
    BINARY_OPERATOR_ILLEGAL(LiteralExpr, <<)
    BINARY_OPERATOR_ILLEGAL(LiteralExpr, >>)
    UNARY_OPERATOR_ILLEGAL(LiteralExpr, ~)
    BINARY_OPERATOR_CONST_RET(LiteralExpr, ==, {
        NumericExpr zero(this, NumericExpr::INTEGER, 0);
        return *c == &zero;
    })
    BINARY_OPERATOR_CONST_RET(LiteralExpr, !=, {
        NumericExpr zero(this, NumericExpr::INTEGER, 0);
        return *c != &zero;
    })
    BINARY_OPERATOR_CONST_RET(LiteralExpr, &&, {
        NumericExpr zero(this, NumericExpr::INTEGER, 0);
        return *c != &zero;
    })
    BINARY_OPERATOR_CONST_RET(LiteralExpr, ||, {
        NumericExpr _true(this, NumericExpr::INTEGER, 1);
        return _true.get_constant(nullptr);
    })
    UNARY_OPERATOR_CONST_RET(LiteralExpr, !, {
        NumericExpr _false(this, NumericExpr::INTEGER, 0);
        return _false.get_constant(nullptr);
    })

    BINARY_OPERATOR_CONST_RET(ConstantExpr, +, return *constant + c;)
    BINARY_OPERATOR_CONST_RET(ConstantExpr, -, return *constant - c;)
    BINARY_OPERATOR_CONST_RET(ConstantExpr, /, return *constant / c;)
    BINARY_OPERATOR_CONST_RET(ConstantExpr, *, return *constant * c;)
    BINARY_OPERATOR_CONST_RET(ConstantExpr, <, return *constant < c;)
    BINARY_OPERATOR_CONST_RET(ConstantExpr, >, return *constant > c;)
    BINARY_OPERATOR_CONST_RET(ConstantExpr, <=, return *constant <= c;)
    BINARY_OPERATOR_CONST_RET(ConstantExpr, >=, return *constant >= c;)
    BINARY_OPERATOR_CONST_RET(ConstantExpr, ==, return *constant == c;)
    BINARY_OPERATOR_CONST_RET(ConstantExpr, !=, return *constant != c;)
    BINARY_OPERATOR_CONST_RET(ConstantExpr, &&, return *constant && c;)
    BINARY_OPERATOR_CONST_RET(ConstantExpr, ||, return *constant || c;)
    BINARY_OPERATOR_CONST_RET(ConstantExpr, &, return *constant & c;)
    BINARY_OPERATOR_CONST_RET(ConstantExpr, |, return *constant | c;)
    BINARY_OPERATOR_CONST_RET(ConstantExpr, ^, return *constant ^ c;)
    BINARY_OPERATOR_CONST_RET(ConstantExpr, <<, return *constant << c;)
    BINARY_OPERATOR_CONST_RET(ConstantExpr, >>, return *constant >> c;)
    UNARY_OPERATOR_CONST_RET(ConstantExpr, ~, return ~*constant;)
    UNARY_OPERATOR_CONST_RET(ConstantExpr, !, return !*constant;)

    Constant* NumericExpr::operator~() const
    {
        if (type == FLOATING)
        {
            throw ASTException(this, "Invalid unary expression on floating point literal");
        }

        return new NumericExpr(this, INTEGER, ~value.integer);
    }

    Constant* NumericExpr::operator!() const
    {
        if (type == FLOATING)
        {
            throw ASTException(this, "Invalid unary expression on floating point literal");
        }

        return new NumericExpr(this, INTEGER, !value.integer);
    }

    Constant* VariableExpr::get_constant(Context* ctx) const
    {
        // TODO Support extrapolating from 'const'
        throw ASTException(this, "Constant expressions cannot have variables");
    }

    Constant* AssignExpr::get_constant(Context* ctx) const
    {
        throw ASTException(this, "Assign expressions are not Constant");
    }

    Constant* CallExpr::get_constant(Context* ctx) const
    {
        throw ASTException(this, "Call expressions are not Constant");
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
            throw ASTException(this, "Undeclared function: " + function);
        }

        std::vector<const IR*> args_ir;
        for (const CallArguments* arg = arguments; arg; arg = arg->next)
        {
            args_ir.push_back(arg->value->get(ctx, IRB));
        }

        F->check_arguments(this, args_ir);
        return IRB.add<CallInstr>(F, args_ir);
    }

    const IR* AssignExpr::get(Context* ctx, IRBuilder &IRB) const
    {
        const IR* out = value->get(ctx, IRB);
        const IR* sink_val = sink->get(ctx, IRB);
        auto* ref = dynamic_cast<const Reference*>(sink_val);
        if (!ref)
        {
            throw ASTException(sink, "Expression does not return a reference");
        }

        IRB.add<MovInstr>(ref, out);
        return out;
    }

    void ASTFunctionDefine::add(Context* ctx, IRBuilder &IRB) const
    {
        ctx->enter_scope(Scope::FUNCTION, name);
        auto* f = dynamic_cast<Function*>(symbol);
        assert(f);

        ctx->set_function(f);
        f->set_entry_block(ctx->scope()->new_block("entry"));
        IRB.set_insertion_point(f->get_entry_block());

        /* Declare the arguments */
        for (Arguments* iter = args; iter; iter = iter->next)
        {
            iter->decl->variable->set(IRB.add<AllocaInstr>(iter->decl->variable));
        }

        /* Add the function code */
        body->add(ctx, IRB);

        Block* final_block = IRB.get_insertion_point();

        if (f->get_destructor_blocks().find(final_block)
            == f->get_destructor_blocks().end())
        {
            if (f->get_return_type() != ctx->type<Type::VOID>())
            {
                // Normal C would just warn here
                // C++ errors here, I think it should be an error
                ctx->emit_error(&end_position, "No return statement at the end of non-void function");
            }

            IRB.add<ReturnInstr>(nullptr);
        }

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
            IRB.add<MovInstr>(gv, initializer);
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

    const Type* CallInstr::get_type(Context* ctx) const
    {
        return f->get_return_type();
    }
}
