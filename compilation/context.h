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

        const Reference* get() const
        {
            assert(value && "Variable not set() yet");
            return value;
        }

        const TypeDecl* get_decl() const
        {
            return declaration;
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

        Scope* get_enter_scope() const { return first_child ? first_child : younger_sibling; }
        Scope* get_exit_scope() const { return older_sibling ? older_sibling : parent; }

        std::string get_lineage() const;
        Block* new_block(const std::string& name = "");
        Block* get_entry_block() const { return blocks.at(0); }
        uint32_t block_count() const { return blocks.size(); }
        const std::vector<Block*>& get_blocks() const { return blocks; }
        Scope* next() const { return younger_sibling; }
        Scope* child() const { return first_child; }

        virtual LoopScope* get_loop();

        Block* get_exit();
        void set_exit(Block* block);

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

        std::string scope_name;
        Block* exit;
    };

    struct LoopScope : public Scope
    {
        LoopScope(Context* ctx, Scope* parent, Scope* older_sibling);
        LoopScope* get_loop() override { return this; }
    };

    class Context
    {
        Module* module;
        Function* function;
        Scope* head;
        Scope* tail;
        bool build_scope;

        std::vector<ASTException> errors;
        std::vector<ASTException> warnings;

        std::unordered_map<std::string, Type*> complex_types;

        Type* primitives[Type::P_N]{nullptr};
        std::vector<Type*> extra_types;
    public:
        Context();

        void register_type(Type* type) { extra_types.push_back(type); }
        Variable* get_variable(const std::string& name) const;
        Variable* declare_variable(TypeDecl* decl);
        const Type* declare_structure(StructDecl* structure);

        Scope* scope() { return tail; }

        Module* get_module() { return module; }
        void set_function(Function* f) { function = f; }
        Function* get_function() { return function; }

        void enter_scope(Scope::scope_t type, const std::string &name = "");
        void exit_scope();

        template<Type::primitive_t T>
        const Type* type() const
        {
            return primitives[T];
        }

        const Type* type(const std::string &name) const
        {
            if (complex_types.find(name) == complex_types.end())
            {
                return nullptr;
            }

            return complex_types.at(name);
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
