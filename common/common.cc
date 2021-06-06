#include "common.h"
#include <sstream>
#include <compilation/module.h>

namespace cc
{
    std::vector<std::string> split_string(const std::string &str, char delim)
    {
        std::stringstream ss(str);
        std::string item;
        std::vector<std::string> elems;

        while (std::getline(ss, item, delim))
        {
            elems.push_back(std::move(item));
        }

        return elems;
    }

    static const Type* get_preferred_type_single(const Type* type_1, const Type* type_2)
    {
        return nullptr;
    }

    const Type* IR::get_preferred_type(std::initializer_list<const IR*> irs)
    {
        const Type* out = nullptr;
        for (const IR* ir : irs)
        {
            if (not out)
            {
                out = ir->get_type(nullptr);
            }
            else
            {
                out = get_preferred_type_single(out, ir->get_type(nullptr));
            }
        }

        return out;
    }

    const Type* Reference::get_type(Context* ctx) const
    {
        return variable->get_decl()->type;
    }
}
