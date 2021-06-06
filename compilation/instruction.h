#ifndef CC_INSTRUCTION_H
#define CC_INSTRUCTION_H

#include <cc.h>
#include <list>
#include <map>
#include <utility>

#include "context.h"

namespace cc
{
    struct Instruction : public IR
    {
        virtual std::string get_name() const = 0;
    };

    class Block : public Value
    {
        Scope* scope;
        Block* next_;
        std::list<Instruction*> instructions;
        std::list<IR*> dangling;
        std::string name;

    public:
        explicit Block(Scope* scope, const std::string& name_ = "")
        : scope(scope), next_(nullptr)
        {
            uint32_t b_c = scope->block_count();
            if (name_.empty())
            {
                if (b_c)
                {
                    name = scope->get_lineage() + "." + std::to_string(b_c);
                }
                else
                {
                    name = scope->get_lineage();
                }
            }
            else
            {
                name = scope->get_lineage() + "." + name_ + "." + std::to_string(b_c);
            }
        }

        void push(Instruction* instruction)
        {
            instructions.push_back(instruction);
        }

        void push_dangling(IR* dangle)
        {
            dangling.push_back(dangle);
        }

        void chain(Block* block, bool force=false)
        {
            if (not force)
            {
                assert(!next_ && "Block already has a next!");
            }

            next_ = block;
        }

        Block* next() const { return next_; }
        std::string get_name() const { return name; }
        std::list<Instruction*>::const_iterator begin() const { return instructions.begin(); }
        std::list<Instruction*>::const_iterator end() const { return instructions.end(); }


        ~Block() override;
    };

    class IRBuilder : public Value
    {
        Block* block;

    public:
        explicit IRBuilder() : block(nullptr) {}

        template<typename T,
                typename... Args,
                typename = typename std::enable_if<std::is_base_of<Instruction, T>::value >::type >
        const T* add(Args... args)
        {
            assert(block && "Attempting to add instruction to nothing");
            T* instr = new T(args...);
            block->push(instr);
            return instr;
        };

        template<typename T,
                typename... Args,
                typename = typename std::enable_if<std::is_base_of<IR, T>::value >::type >
        const T* add_dangling(Args... args)
        {
            assert(block && "Attempting to add instruction to nothing");
            T* ir = new T(args...);
            block->push_dangling(ir);
            return ir;
        }

        void set_insertion_point(Block* block_) { block = block_; }
        Block* get_insertion_point() { return block; }
    };

    /****************************************************************************
     *                                                                          *
     *                          Instruction definition                          *
     *                                                                          *
     ****************************************************************************/

    struct BinaryInstr : public Instruction
    {
        const IR* a;
        const IR* b;

        const Type* get_type(Context* ctx) const override { return get_preferred_type({a, b}); }
        BinaryInstr(const IR* a, const IR* b) : a(a), b(b) {}
    };

    struct UnaryInstr : public Instruction
    {
        const IR* v;
        const Type* get_type(Context* ctx) const override { return v->get_type(ctx); }
        explicit UnaryInstr(const IR* v) : v(v) {}
    };

#define BINARY_INSTR(name) struct name##Instr : public BinaryInstr \
    {                                                              \
        name##Instr(const IR* a, const IR* b) : BinaryInstr(a, b)  {}          \
        std::string get_name() const override { return #name "Instr"; } \
    }

#define UNARY_INSTR(name)   \
    struct name##Instr : public UnaryInstr                  \
    {                                                       \
        explicit name##Instr(const IR* v) : UnaryInstr(v)  {}     \
        std::string get_name() const override { return #name "Instr"; } \
    }

    /** Arithmetic instructions **/
    BINARY_INSTR(Add);
    BINARY_INSTR(Sub);
    BINARY_INSTR(Div);
    BINARY_INSTR(Mul);
    UNARY_INSTR(Inc);
    UNARY_INSTR(Dec);

    /** ====================== **/

    /** Logical instructions **/
    UNARY_INSTR(L_Not);
    BINARY_INSTR(L_And);
    BINARY_INSTR(L_Or);
    BINARY_INSTR(LT);
    BINARY_INSTR(GT);
    BINARY_INSTR(LE);
    BINARY_INSTR(GE);
    BINARY_INSTR(EQ);

    /** Bitwise instructions **/
    UNARY_INSTR(B_Not);
    BINARY_INSTR(B_And);
    BINARY_INSTR(B_Or);
    BINARY_INSTR(B_Xor);
    BINARY_INSTR(L_SL);
    BINARY_INSTR(L_SR);
    BINARY_INSTR(A_SR);

    /** Jumping and Branching **/
    struct JumpInstr : public Instruction
    {
        /**
         * Unconditional jump
         */

        Block* target;

        explicit JumpInstr(Block* target) : target(target) {}
        std::string get_name() const override { return "JumpInstr"; }
        const Type* get_type(Context* ctx) const override { return ctx->type<Type::VOID>(); }
    };

    struct BranchInstr : public JumpInstr
    {
        /**
         * Conditional jump
         */

        const IR* condition;

        explicit BranchInstr(Block* target, const IR* condition) :
            JumpInstr(target), condition(condition) {}
        std::string get_name() const override { return "BranchInstr"; }
    };

    /** Misc **/

    struct AllocaInstr : public Reference, public Instruction
    {
        explicit AllocaInstr(Variable* variable) : Reference(variable) {}
        std::string get_name() const override { return "AllocaInstr"; }
        const Type* get_type(Context* ctx) const override { return Reference::get_type(ctx); }
    };

    struct MovInstr : public Instruction
    {
        const Reference* dest;
        const IR* src;

        MovInstr(const Reference* dest, const IR* src) : dest(dest), src(src) {}
        std::string get_name() const override { return "MovInstr"; }
        const Type* get_type(Context* ctx) const override { return dest->get_type(ctx); };
    };

    struct ReturnInstr : public Instruction
    {
        const IR* return_value;
        ReturnInstr(const IR* return_value) : return_value(return_value) {}

        std::string get_name() const override { return "ReturnInstr"; }
        const Type* get_type(Context* ctx) const override { return ctx->type<Type::VOID>(); }
    };

    struct CallInstr : public Instruction
    {
        const Function* f;
        std::vector<const IR*> arguments;
        CallInstr(const Function* F, std::vector<const IR*> arguments) : f(F), arguments(std::move(arguments)) {}
        std::string get_name() const override { return "CallInstr"; }
        const Type* get_type(Context* ctx) const override;
    };
}

#endif //CC_INSTRUCTION_H
