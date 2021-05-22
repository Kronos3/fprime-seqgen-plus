#ifndef CC_MODULE_H
#define CC_MODULE_H

#include <cc.h>
#include <map>
#include "context.h"

namespace cc
{
    class Global : public IR
    {
        // TODO Implement linkage types

        const Type* type;
        std::string name;

    protected:
        explicit Global(std::string name, const Type* type) :
        name(std::move(name)), type(type) {}

    public:
        std::string get_name() const { return name; }
    };

    class GlobalVariable : public Global, public Buildable
    {
        void add(Context* ctx, IRBuilder &IRB) const override;

    public:
        explicit GlobalVariable(ASTGlobalVariable* ast) :
        Buildable({ast}), Global(ast->decl->name, ast->decl->type) {}
    };

    class Function : public Global, public Type
    {
        /**
         * Models a functions call signature
         */

        const Type* return_type;
        std::vector<const Type*> signature;

    public:
        explicit Function(ASTFunction* ast) :
                 Global(ast->name, this),
                 Type(ast->return_type->get_ctx(), Type::FUNCTION),
                 return_type(ast->return_type), signature()
        {
            for (Arguments* iter = ast->args; iter; iter = iter->next)
            {
                signature.push_back(iter->decl->type);
            }
        }

        const std::vector<const Type*>& get_signature() const { return signature; };
        const Type* get_return_type() const { return return_type; }
    };

    class Module : Value
    {
        Scope* global_scope;

        // Global variables and functions
        std::map<std::string, Global*> symbols;

        bool declare_symbol(Global* self);
        Context* ctx;

    public:
        explicit Module(Context* context) :
        global_scope(Scope::create(Scope::GLOBAL, context)),
        ctx(context) {}

        GlobalVariable* declare_variable(ASTGlobalVariable* variable);
        Function* declare_function(ASTFunction* variable);

        Scope* scope() const { return global_scope; }

        ~Module() override;
    };
}

#endif //CC_MODULE_H
