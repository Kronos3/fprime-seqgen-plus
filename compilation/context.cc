#include <sstream>
#include "context.h"
#include "instruction.h"
#include "module.h"

namespace cc
{
    Variable* Scope::declare_variable(TypeDecl* decl)
    {
        if (get_variable(decl->name))
        {
            ctx->emit_error(decl, "Duplicate variable definition of " + decl->name);
            return nullptr;
        }

        auto* new_var = new Variable(decl);
        variables[decl->name] = new_var;
        return new_var;
    }

    Variable* Scope::get_variable(const std::string& name) const
    {
        // Keep travelling to outer scopes until we find the variable
        for (const Scope* iter = this; iter; iter = iter->get_exit())
        {
            if (iter->variables.find(name) != iter->variables.end())
            {
                return iter->variables.at(name);
            }
        }

        return nullptr;
    }

    Scope* Scope::add_child(const std::string& name_, Scope::scope_t type_)
    {
        auto* newborn = Scope::create(type_, ctx, name_, this, last_child);
        if (!last_child)
        {
            last_child = newborn;
            first_child = newborn;
        }
        else
        {
            last_child->younger_sibling = newborn;
            last_child = newborn;
        }
        return newborn;
    }

    std::string Scope::get_lineage() const
    {
        // TODO Remove recursion

        if (parent)
        {
            return parent->get_lineage() + "." + scope_name;
        }
        else
        {
            return scope_name;
        }
    }

    Scope::Scope(Context* ctx, std::string name, Scope* parent, Scope* older_sibling) :
        ctx(ctx), parent(parent), older_sibling(older_sibling),
        first_child(nullptr), last_child(nullptr),
        scope_name(std::move(name)), younger_sibling(nullptr)
    {
        entry_block = new_block();
    }

    Block* Scope::new_block()
    {
        auto* b = new Block(this);
        blocks.push_back(b);
        return b;
    }

    Scope::~Scope()
    {
        delete first_child;
        delete younger_sibling;

        for (auto& iter : variables)
        {
            delete iter.second;
        }

        variables.clear();

        for (auto* b : blocks)
        {
            delete b;
        }

        blocks.clear();
    }

    LoopScope* Scope::get_loop()
    {
        Scope* iter = this;
        for (; !dynamic_cast<LoopScope*>(iter); iter = iter->parent)
        {
            if (!iter)
            {
                break;
            }
        }

        return dynamic_cast<LoopScope*>(iter);
    }

    Scope* Scope::create(scope_t type, Context* ctx,
                         const std::string &name,
                         Scope* parent,
                         Scope* older_sibling)
    {
        switch (type)
        {
            case GLOBAL: return new Scope(ctx, "<top>", nullptr, nullptr);
            case BRACKET:
                assert(parent);
                return new Scope(ctx, parent, older_sibling);
            case FUNCTION:
                assert(!name.empty());
                return new Scope(ctx, name, parent, older_sibling);
            case LOOP:
                assert(parent);
                return new LoopScope(ctx, parent, older_sibling);
        }

        return nullptr;
    }

    static int loop_scope_counter = 0;
    static int scope_count = 0;
    Scope::Scope(Context* ctx, Scope* parent, Scope* older_sibling) :
    Scope(ctx, variadic_string("scope-%d", scope_count++), parent, older_sibling)
    {
    }

    LoopScope::LoopScope(Context* ctx, Scope* parent, Scope* older_sibling) :
            Scope(ctx, variadic_string("loop-%d", loop_scope_counter++), parent, older_sibling),
            exit(nullptr)
    {
    }

    void LoopScope::set_exit(Block* block)
    {
        assert(!exit && "Loop exit already set");
        exit = block;
    }

    Block* LoopScope::get_exit()
    {
        assert(exit && "Failed to get loop exit block");
        return exit;
    }

    Variable* Context::get_variable(const std::string& name) const
    {
        return tail->get_variable(name);
    }

    Variable* Context::declare_variable(TypeDecl* decl)
    {
        if (get_variable(decl->name))
        {
            return nullptr;
        }

        return tail->declare_variable(decl);
    }

    void Context::enter_scope(Scope::scope_t type, const std::string &name)
    {
        assert(head);
        assert(tail);

        if (build_scope)
        {
            tail = tail->add_child(name, type);
        }
        else
        {
            tail = tail->get_enter();
            assert(tail && "Scope skeleton not built yet!");
        }
    }

    void Context::exit_scope()
    {
        assert(tail && "No more scopes to exit");
        tail = tail->get_parent();
    }

    Context::~Context()
    {
        delete module;
        delete primitives[Type::VOID];
        delete primitives[Type::CHAR];
        delete primitives[Type::I8];
        delete primitives[Type::I16];
        delete primitives[Type::I32];
        delete primitives[Type::I64];
        delete primitives[Type::F32];
        delete primitives[Type::F64];
        delete primitives[Type::PTR];
    }

    Context::Context() :
    module(new Module(this)), head(module->scope()),
    tail(head), build_scope(false)
    {
        primitives[Type::VOID] = new PrimitiveType<Type::VOID>(this);
        primitives[Type::CHAR] = new PrimitiveType<Type::CHAR>(this);
        primitives[Type::I8] = new PrimitiveType<Type::I8>(this);
        primitives[Type::I16] = new PrimitiveType<Type::I16>(this);
        primitives[Type::I32] = new PrimitiveType<Type::I32>(this);
        primitives[Type::I64] = new PrimitiveType<Type::I64>(this);
        primitives[Type::F32] = new PrimitiveType<Type::F32>(this);
        primitives[Type::F64] = new PrimitiveType<Type::F64>(this);
        primitives[Type::PTR] = new PrimitiveType<Type::PTR>(this);
    }
}
