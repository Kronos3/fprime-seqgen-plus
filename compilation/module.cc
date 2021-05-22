#include "module.h"

namespace cc
{
    GlobalVariable* Module::declare_variable(ASTGlobalVariable* variable)
    {
        auto* gv = new GlobalVariable(variable);
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

    }

    void GlobalVariable::add(Context* ctx, IRBuilder &IRB) const
    {
    }
}