#pragma once

#include <cstdint>
#include <optional>
#include <string>
#include <map>
#include <vector>

enum value_type_t
{
    NONE,
    UINT,
    INT,
    FLOAT,
    STRING
};

struct argument_value_t
{
    value_type_t type;
    std::uint32_t   u;
    std::int32_t    i;
    float           f;
    std::string     s;
};

std::optional<std::map<std::string, std::vector<argument_value_t>>> parse_command_line_arguments(int argc, char** args,
        std::map<std::string, std::tuple<std::vector<value_type_t>, std::uint32_t>> allowed_args);
