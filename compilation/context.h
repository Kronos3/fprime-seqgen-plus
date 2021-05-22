#ifndef CONTEXT_H
#define CONTEXT_H

#include <cc.h>
#include <stack>
#include <list>
#include <cassert>

namespace cc
{
    class Variable : public Value
    {
        TypeDecl* declaration;
    public:
        explicit Variable(TypeDecl* decl) : declaration(decl) {}
    };

    class Scope
    {
        std::unordered_map<std::string, Variable*> variables;
        Scope* parent;
        Scope* first_child;
        Scope* last_child;
        Scope* younger_sibling;
        Scope* older_sibling;

        std::string scope_name;

    public:
        Scope* get_parent() const { return parent; }

        Variable* declare_variable(TypeDecl* decl);
        Variable* get_variable(const std::string& name) const;
        Scope(std::string name,
              Scope* parent,
              Scope* older_sibling) :
              parent(parent), older_sibling(older_sibling),
              first_child(nullptr), last_child(nullptr),
              scope_name(std::move(name)),
              younger_sibling(nullptr) {}

        Scope* add_child(const std::string& name);

        ~Scope()
        {
            delete first_child;
            delete younger_sibling;

            for (auto& iter : variables)
            {
                delete iter.second;
            }
            variables.clear();
        }

        Scope* get_enter() const { return first_child ? first_child : younger_sibling; }
        Scope* get_exit() const { return older_sibling ? older_sibling : parent; }

        std::string get_lineage() const;
    };

    class Context
    {
        Scope* head;
        Scope* tail;
        int scope_count;
        bool build_scope;
    public:
        Context()
        : head(nullptr), tail(nullptr), scope_count(0),
            build_scope(false) {}

        Variable* get_variable(const std::string& name) const;
        Variable* declare_variable(TypeDecl* decl);

        void enter_scope(const std::string& name);
        void exit_scope();

        void start_scope_build() { build_scope = true; }
        void end_scope_build() { build_scope = false; }

        ~Context()
        {
            delete head;
        }
    };
}

#endif //CONTEXT_H
