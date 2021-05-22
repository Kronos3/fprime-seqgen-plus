#ifndef CC_INSTRUCTION_H
#define CC_INSTRUCTION_H

#include <cc.h>
#include <list>
#include <map>

#include "context.h"

namespace cc
{
    struct Instruction : public IR
    {
    };

    class Block : public Value
    {
        Scope* scope;
        Block* next;
        std::list<Instruction*> instructions;
        std::list<IR*> dangling;

    public:
        explicit Block(Scope* scope) : scope(scope), next(nullptr) {}

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
            assert(!next && "Block already has a next!");
            next = block;
        }

        ~Block() override;
    };

    class IRBuilder : public Value
    {
        Block* block;

    public:
        explicit IRBuilder(Block* block) : block(block) {}

        template<typename T,
                typename... Args,
                typename = typename std::enable_if<std::is_base_of<Instruction, T>::value >::type >
        T* add(Args... args)
        {
            T* instr = new T(args...);
            block->push(instr);
            return instr;
        };

        template<typename T,
                typename... Args,
                typename = typename std::enable_if<std::is_base_of<IR, T>::value >::type >
        T* add_dangling(Args... args)
        {
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
#define BINARY_INSTR(name) \
    struct name##Instr : public Instruction \
    {   \
        IR* a; \
        IR* b; \
        name##Instr(IR* a, IR* b) : a(b), b(b) {} \
    }

#define UNARY_INSTR(name)   \
    struct name##Instr : public Instruction \
    {   \
        IR* v; \
        explicit name##Instr(IR* v) : v(v) {} \
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
    };

    struct BranchInstr : public JumpInstr
    {
        /**
         * Conditional jump
         */

        IR* condition;

        explicit BranchInstr(Block* target, IR* condition, bool inlined = true) :
            JumpInstr(target, inlined), condition(condition) {}
    };

    /** Misc **/

    struct AllocaInstr : public Instruction, public Reference
    {
        const Type* type;
        explicit AllocaInstr(const Type* type) : type(type) {}
    };

    struct MovInstr : public Instruction
    {
        Reference* dest;
        IR* src;

        explicit MovInstr(Reference* dest, IR* src) : dest(dest), src(src) {}
    };

    struct ReturnInstr : public Instruction
    {
    };

    template<unsigned int N>
    struct Immediate : public IR
    {
        static_assert(N == 8 ||
                      N == 16 ||
                      N == 32 ||
                      N == 64, "Invalid immediate bit-size");

        int bit_n = N;
        uint64_t value;

    protected:
        template<typename T>
        explicit Immediate(T value) : bit_n(N), value(0)
        {
            union { T temp; uint64_t real; } u;
            u.temp = value;
            value = u.real;
        }

    };

    template<unsigned int N>
    struct ImmInteger : public Immediate<N>
    {
        template<typename T,
                typename = typename std::enable_if< std::is_integral<T>::value >>
        explicit ImmInteger(T value) : Immediate<N>(value)
        {
        }
    };

    template<unsigned int N>
    struct ImmFloat : public Immediate<N>
    {
        static_assert(N == 32 ||
                      N == 64, "Invalid floating immediate bit-size");

        template<typename T,
                typename = typename std::enable_if< std::is_floating_point<T>::value >>
        explicit ImmFloat(T value) : Immediate<N>(value)
        {
        }
    };

    struct CallInstr : public Instruction
    {

    };
}

#endif //CC_INSTRUCTION_H
