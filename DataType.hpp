#pragma once
#include <string>
enum DataType{
    CHAR,
    INTEGER,
    DOUBLE,
    STRING
};

std::string DataType_ToCode(DataType type)
{
    if (type == DataType::CHAR || type == DataType::INTEGER || type == DataType::STRING)
        return "i32"; // for char or int
    return "f64"; // for double
}
