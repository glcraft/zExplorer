#pragma once
#if defined (__cpp_lib_format) && __cpp_lib_format >= 201907L
#include <format>
namespace cmd {
    using fmt::format;
}
#else
#include <fmt/format.h>
namespace cmd {
    using fmt::format;
}
#endif