#include "common.h"
#include <sstream>

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
}
