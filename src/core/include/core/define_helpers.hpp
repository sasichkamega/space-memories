#pragma once

#include <core/type_index.hpp>

#define CORE_CONCAT_LEXEMS_(a,b) a##b
#define CORE_CONCAT_LEXEMS(a,b) CORE_CONCAT_LEXEMS_(a,b)
#define CORE_GENERATE_UNIQUE_NAME(basename) CORE_CONCAT_LEXEMS(basename,__COUNTER__)