#ifndef CONTEXT_H
#define CONTEXT_H

#include <cc.h>
#include <stack>
#include <list>
#include <cassert>
#include <utility>
#include "type.h"

namespace cc
{
    class Block;
    class Context;
    class Module;

    class Variable : public Value
    {
        TypeDecl* declaration;
        const Reference* value;
    public:
        explicit Variable(TypeDecl* decl) : declaration(decl), value(nullptr) {}



        void set(const Reference* value_)
        {
            assert(!value && "Value already set");
            value = value_;
        }

        const Reference* get()
        {
            assert(value && "Variable not set() yet");
            return value;
        }
    };

    struct LoopScope;

    struct Scope
    {
        /**
         * A scope will keep track of variable declarations
         * as well as hold blocks of instructions. It will
         * provide context for instructions such as `break` and `continue`
         * who need to look at parent scopes to create the correct
         * branching targets.
         *
         * Scopes will create a family tree with a single parent:
         * {
         *     parent
         *     { older_sibling }
         *     {
         *         this
         *         { first_child }
         *         {
         *             other children are referencable from
         *             the first child or last child
         *             ...
         *         }
         *         { last_child }
         *     }
         *     { younger_sibling }
         * }
         */

        typedef enum
        {
            GLOBAL,
            FUNCTION,
            BRACKET,
            LOOP,
        } scope_t;

        Scope* get_parent() const { return parent; }

        Variable* declare_variable(TypeDecl* decl);
        Variable* get_variable(const std::string& name) const;

        static Scope* create(scope_t type, Context* ctx,
                             const std::string& name = "",
                             Scope* parent = nullptr,
                             Scope* older_sibling = nullptr);

        Scope* add_child(const std::string& name, scope_t type_);

        virtual ~Scope();

        Scope* get_enter() const { return first_child ? first_child : younger_sibling; }
        Scope* get_exit() const { return older_sibling ? older_sibling : parent; }

        std::string get_lineage() const;
        Block* new_block();
        Block* get_entry_block() const { return entry_block; }
        uint32_t block_count() const { return blocks.size(); }
        Scope* next() const { return younger_sibling; }
        Scope* child() const { return first_child; }

        virtual LoopScope* get_loop();

    protected:
        Scope(Context* ctx,
              std::string name,
              Scope* parent,
              Scope* older_sibling);

        Scope(Context* ctx,
              Scope* parent,
              Scope* older_sibling);

        std::unordered_map<std::string, Variable*> variables;
        Scope* parent;
        Scope* first_child;
        Scope* last_child;
        Scope* younger_sibling;
        Scope* older_sibling;

        Context* ctx;
        std::vector<Block*> blocks;
        Block* entry_block;

        std::string scope_name;
    };

    struct LoopScope : public Scope
    {
        LoopScope(Context* ctx, Scope* parent, Scope* older_sibling);

        Block* get_exit();
        void set_exit(Block* block);
        LoopScope* get_loop() override { return this; }

    private:
        Block* exit;
    };

    class Context
    {
        Module* module;
        Scope* head;
        Scope* tail;
        bool build_scope;

        std::vector<ASTException> errors;
        std::vector<ASTException> warnings;

        Type* primitives[Type::P_N]{nullptr};
    public:
        Context();

        Variable* get_variable(const std::string& name) const;
        Variable* declare_variable(TypeDecl* decl);
        Scope* scope() { return tail; }

        int get_ptr_size() const { /* TODO get from platform */ return 4; }
        Module* get_module() { return module; }

        void enter_scope(Scope::scope_t type, const std::string &name = "");
        void exit_scope();

        template<Type::primitive_t T>
        const Type* type() const
        {
            static_assert(T < Type::ENUM, "Only primitives are allowed here");
            return primitives[T];
        }

        void start_scope_build() { build_scope = true; }
        void end_scope_build() { build_scope = false; }

        template<typename... Args>
        void emit_error(Args... args) { errors.emplace_back(args...); }

        template<typename... Args>
        void emit_warning(Args... args) { warnings.emplace_back(args...); }
        const std::vector<ASTException>& get_errors() const { return errors; }
        const std::vector<ASTException>& get_warnings() const { return warnings; }
        void clear_warnings() { warnings.clear(); }

        ~Context();
    };
}

#endif //CONTEXT_H
