#ifndef CC_MODULE_H
#define CC_MODULE_H

#include <cc.h>
#include <map>
#include "context.h"

namespace cc
{
    class Global : public Value
    {
    protected:
        // TODO Implement linkage types

        Context* ctx;
        const Type* type;
        std::string name;

        explicit Global(std::string name, const Type* type, Context* ctx) :
        name(std::move(name)), type(type), ctx(ctx) {}

    public:
        std::string get_name() const { return name; }
    };

    class GlobalVariable : public Global, public Reference
    {
    public:
        explicit GlobalVariable(ASTGlobalVariable* ast) :
        Global(ast->decl->name, ast->decl->type, ast->decl->type->get_ctx()) {}

        GlobalVariable(const std::string& name, const Type* type)
        : Global(name, type, type->get_ctx()) {}
    };

    class ConstantGlobal : public GlobalVariable
    {
        const Constant* value;
    public:
        ConstantGlobal(Context* ctx, const std::string& name, const ASTConstant* ast) :
                GlobalVariable(name, ctx->type<Type::PTR>()), value(ast) {}
    };

    class Function : public Global
    {
        /**
         * Models a functions call signature
         */

        const Type* return_type;
        std::vector<const Type*> signature;
        Block* entry;
        const ASTFunction* ast;

    public:
        explicit Function(ASTFunction* ast) :
                 Global(ast->name, nullptr, ast->return_type->get_ctx()), ast(ast),
                 return_type(ast->return_type), signature(), entry(nullptr)
        {
            for (Arguments* iter = ast->args; iter; iter = iter->next)
            {
                signature.push_back(iter->decl->type);
            }
        }

        bool check_arguments(const CallExpr* call, const std::vector<IR*>& args) const;

        Block* get_entry_block() const { return entry; }
        void set_entry_block(Block* block) { entry = block; }
        const std::vector<const Type*>& get_signature() const { return signature; };
        const Type* get_return_type() const { return return_type; }
        const ASTFunction* get_ast() const { return ast; }
    };

    class Module : public Value
    {
        Block* constructor_block;
        Block* destructor_block;

        Scope* global_scope;

        // Global variables and functions
        std::map<std::string, Global*> symbols;

        bool declare_symbol(Global* self);
        Context* ctx;

    public:
        explicit Module(Context* context);

        GlobalVariable* declare_variable(ASTGlobalVariable* variable);
        Function* declare_function(ASTFunction* variable);
        const Global* get_symbol(const std::string& name) const;
        const Function* get_function(const std::string& name)
        { return dynamic_cast<const Function*>(get_symbol(name)); }

        Scope* scope() const { return global_scope; }
        Block* constructor() const { return constructor_block; }
        Block* destructor() const { return destructor_block; }

        ~Module() override;
    };
}

#endif //CC_MODULE_H
