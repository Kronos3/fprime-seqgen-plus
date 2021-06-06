#include "module.h"
#include "context.h"

namespace cc
{
    GlobalVariable* Module::declare_variable(ASTGlobalVariable* variable)
    {
        auto* gv = new GlobalVariable(nullptr, variable);
        if (!declare_symbol(gv))
        {
            ctx->emit_error(variable, "Duplicate global symbol " + gv->get_name());
            return nullptr;
        }

        return gv;
    }

    Function* Module::declare_function(ASTFunction* variable)
    {
        auto* f = new Function(variable);
        if (!declare_symbol(f))
        {
            ctx->emit_error(variable, "Duplicate global symbol " + f->get_name());
            return nullptr;
        }

        return f;
    }

    bool Module::declare_symbol(Global* self)
    {
        if (symbols.find(self->get_name()) != symbols.end())
        {
            return false;
        }

        symbols[self->get_name()] = self;
        return true;
    }

    Module::~Module()
    {
        delete global_scope;
        for (auto& iter : symbols)
        {
            delete iter.second;
        }
        symbols.clear();
    }

    Module::Module(Context* context) :
            global_scope(Scope::create(Scope::GLOBAL, context)),
            constructor_block(nullptr), destructor_block(nullptr),
            ctx(context)
    {
        constructor_block = global_scope->new_block("constructor");
        destructor_block = global_scope->new_block("destructor");
    }

    const Global* Module::get_symbol(const std::string& name) const
    {
        for (const auto& iter : symbols)
        {
            if (iter.second->get_name() == name)
            {
                return iter.second;
            }
        }

        return nullptr;
    }

    bool Function::check_arguments(const CallExpr* call, const std::vector<const IR*>& args) const
    {
        /* TODO implement IR type checking */

        if (args.size() != signature.size())
        {
            ctx->emit_error(call,
                            variadic_string("Function %s expects %d arguments, got %d",
                                            name.c_str(), signature.size(), args.size()));
            return false;
        }

        return true;
    }
}