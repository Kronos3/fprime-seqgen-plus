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
        explicit Block(Scope* scope)
        : scope(scope), next_(nullptr)
        {
            uint32_t b_c = scope->block_count();
            if (b_c)
            {
                name = variadic_string("%s.%d", scope->get_lineage().c_str(), b_c);
            }
            else
            {
                name = scope->get_lineage();
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

        void chain(Block* block)
        {
            assert(!next_ && "Block already has a next!");
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
     *
     *                          Instruction definition                          *
     *
     ****************************************************************************/

    struct BinaryInstr : public Instruction
    {
        const IR* a;
        const IR* b;
        BinaryInstr(const IR* a, const IR* b) : a(b), b(b) {}
    };

    struct UnaryInstr : public Instruction
    {
        const IR* v;
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

    /** Jumping and Branching **/
    struct JumpInstr : public Instruction
    {
        /**
         * Unconditional jump
         */

        Block* target;
        bool inlined;

        explicit JumpInstr(Block* target, bool inlined = false) : target(target), inlined(inlined) {}
        std::string get_name() const override { return "JumpInstr"; }
    };

    struct BranchInstr : public JumpInstr
    {
        /**
         * Conditional jump
         */

        const IR* condition;

        explicit BranchInstr(Block* target, const IR* condition, bool inlined = true) :
            JumpInstr(target, inlined), condition(condition) {}
        std::string get_name() const override { return "BranchInstr"; }
    };

    /** Misc **/

    struct AllocaInstr : public Instruction, public Reference
    {
        const Type* type;
        explicit AllocaInstr(const Type* type) : type(type) {}
        std::string get_name() const override { return "AllocaInstr"; }
    };

    struct MovInstr : public Instruction
    {
        const Reference* dest;
        const IR* src;

        explicit MovInstr(const Reference* dest, const IR* src) : dest(dest), src(src) {}
        std::string get_name() const override { return "MovInstr"; }
    };

    struct ReturnInstr : public Instruction
    {
        std::string get_name() const override { return "ReturnInstr"; }
    };

    struct CallInstr : public Instruction
    {
        const Function* f;
        std::vector<const IR*> arguments;
        CallInstr(const Function* F, std::vector<const IR*> arguments) : f(F), arguments(std::move(arguments)) {}
        std::string get_name() const override { return "CallInstr"; }
    };
}

#endif //CC_INSTRUCTION_H
