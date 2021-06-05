#include "print_debug.h"
#include "compilation/module.h"

namespace cc
{
    std::stringstream& p(std::stringstream &ss, const Reference* self)
    {
        (void) self;
        return ss;
    }

    std::stringstream& p(std::stringstream &ss, const Constant* self)
    {
        if (dynamic_cast<const LiteralExpr*>(self))
        {
            ss << "\"" << dynamic_cast<const LiteralExpr*>(self)->value << "\"";
        }
        else if (dynamic_cast<const NumericExpr*>(self))
        {
            ss << dynamic_cast<const NumericExpr*>(self)->as_string();
        }
        else
        {

        }

        return ss;
    }

    std::stringstream& p(std::stringstream &ss, const Instruction* self)
    {
        ss << self->as_string() << " = " << self->get_name() << "[";
        if (dynamic_cast<const BinaryInstr*>(self))
        {
            const auto* self_ = dynamic_cast<const BinaryInstr*>(self);
            ss << self_->a->as_string() << ", "
               << self_->b->as_string();
        }
        else if (dynamic_cast<const UnaryInstr*>(self))
        {
            const auto* self_ = dynamic_cast<const UnaryInstr*>(self);
            ss << self_->v->as_string();
        }
        else if (dynamic_cast<const AllocaInstr*>(self))
        {
            const auto* self_ = dynamic_cast<const AllocaInstr*>(self);
            ss << self_->type->as_string();
        }
        else if (dynamic_cast<const BranchInstr*>(self))
        {
            const auto* self_ = dynamic_cast<const BranchInstr*>(self);
            ss << "cond=" << self_->condition->as_string() << ", "
               << "target=" << self_->target->get_name() << ", "
               << "inline=" << self_->inlined;
        }
        else if (dynamic_cast<const JumpInstr*>(self))
        {
            const auto* self_ = dynamic_cast<const JumpInstr*>(self);
            ss << "target=" << self_->target->get_name() << ", "
               << "inline=" << self_->inlined;
        }
        else if (dynamic_cast<const MovInstr*>(self))
        {
            const auto* self_ = dynamic_cast<const MovInstr*>(self);
            ss << self_->dest->as_string() << ", "
               << self_->src->as_string();
        }
        else if (dynamic_cast<const CallInstr*>(self))
        {
            const auto* self_ = dynamic_cast<const CallInstr*>(self);
            ss << self_->f->get_name() << " ";
            for (int i = 0; i < self_->arguments.size(); i++)
            {
                ss << "[" << self_->f->get_signature()[i]->as_string() << "] "
                   << self_->arguments[i]->as_string();
                if (i + 1 < self_->arguments.size())
                {
                    ss << ", ";
                }
            }
        }

        ss << "]";
        return ss;
    }

    std::stringstream& p(std::stringstream& ss, const Block* self)
    {
        ss << self->get_name() << ":\n";
        for (auto iter : *self)
        {
            p(ss, iter) << "\n";
        }
        return ss;
    }

    std::stringstream& p(std::stringstream& ss, const Scope* self)
    {
        for (const Scope* s_iter = self; s_iter; s_iter = s_iter->next())
        {
            for (const auto& iter : s_iter->get_blocks())
            {
                p(ss, iter);
                if (iter->next())
                {
                    ss << "goto " << iter->next()->get_name() << "\n\n";
                }
                else
                {
                    ss << "end\n\n";
                }
            }

            if (self->child())
            {
                p(ss, self->child());
            }
        }

        return ss;
    }
}