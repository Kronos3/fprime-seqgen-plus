#include <sstream>
#include "context.h"

namespace cc
{
    Variable* Scope::declare_variable(TypeDecl* decl)
    {
        if (get_variable(decl->name))
        {
            throw Exception("Duplicate variable definition of " +
                            decl->name);
        }

        auto* new_var = new Variable(decl);
        variables[decl->name] = new_var;
        return new_var;
    }

    Variable* Scope::get_variable(const std::string& name) const
    {
        if (variables.find(name) != variables.end())
        {
            return variables.at(name);
        }

        return nullptr;
    }

    Scope* Scope::add_child(const std::string& name_)
    {
        auto* newborn = new Scope(name_, this, last_child);
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
        if (parent)
        {
            return parent->get_lineage() + "." + scope_name;
        }
        else
        {
            return scope_name;
        }
    }

    Variable* Context::get_variable(const std::string& name) const
    {
        // Keep travelling to outer scopes until we find the variable
        for (Scope* iter = tail; iter; iter = iter->get_exit())
        {
            Variable* v;
            if ((v = iter->get_variable(name)))
            {
                return v;
            }
        }

        return nullptr;
    }

    Variable* Context::declare_variable(TypeDecl* decl)
    {
        if (get_variable(decl->name))
        {
            throw ASTException(decl, "Duplicate variable definition of " + decl->name);
        }

        return tail->declare_variable(decl);
    }

    void Context::enter_scope(const std::string &name)
    {
        if (!head)
        {
            head = new Scope("<top>", nullptr, nullptr);
            tail = head;
        }

        if (tail)
        {
            if (build_scope)
            {
                tail = tail->add_child(name.empty() ? std::to_string(scope_count) : name);
            }
            else
            {
                tail = tail->get_enter();
                assert(tail && "Scope skeleton not built yet!");
            }
        }
        else
        {
            tail = head;
        }
    }

    void Context::exit_scope()
    {
        assert(tail && "No more scopes to exit");
        tail = tail->get_parent();
    }
}
