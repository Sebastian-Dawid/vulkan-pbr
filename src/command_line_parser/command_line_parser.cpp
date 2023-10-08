#include "command_line_parser.h"
#include <algorithm>
#include <cstring>
#include <vector>
#include <iostream>

std::optional<std::map<std::string, std::vector<argument_value_t>>> parse_command_line_arguments(int argc, char **args,
        std::map<std::string, std::tuple<std::vector<value_type_t>, std::uint32_t>> allowed_args)
{
    std::map<std::string, std::vector<argument_value_t>> result;

    std::vector<char*> args_vec(args, args + argc);
#ifdef DEBUG
    std::for_each(args_vec.begin(), args_vec.end(), [](char* e){ std::cout << e << std::endl; });
#endif
    for (std::uint32_t i = 0; i < argc; ++i)
    {
        auto iter = std::find_if(allowed_args.begin(), allowed_args.end(), [&] (const auto& e) { return std::strcmp(e.first.c_str(), args_vec[i]) == 0; });
        if (iter != allowed_args.end())
        {
#ifdef DEBUG
            std::cout << args_vec[i] << " found" << std::endl;
#endif
            std::uint32_t arg_count = std::get<1>(iter->second);
            if (i + arg_count + 1 > argc)
            {
                return std::nullopt;
            }
            std::vector<argument_value_t> values(arg_count);
            if (arg_count > 0)
            {
                for (std::uint32_t j = 0; j < arg_count; ++j)
                {
                    values[j].type = std::get<0>(iter->second)[j];
                    switch (values[j].type)
                    {
                        case value_type_t::UINT:
                            values[j].u = (std::uint32_t) std::atoi(args_vec[i + j + 1]);
                            break;
                        case value_type_t::INT:
                            values[j].i = std::atoi(args_vec[i + j + 1]);
                            break;
                        case value_type_t::FLOAT:
                            values[j].f = std::atof(args_vec[i + j + 1]);
                            break;
                        case value_type_t::STRING:
                            values[j].s = args_vec[i + j + 1];
                            break;
                        default:
                            break;
                    }
                }
            }
            result[args_vec[i]] = values;
            i += arg_count;
        }
    }

    return result;
}
