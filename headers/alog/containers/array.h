#pragma once
#include <alog/record.h>
#include <array>

template<typename Arg, size_t N>
struct ALog::Internal::is_container<std::array<Arg, N>> : public std::true_type { };
