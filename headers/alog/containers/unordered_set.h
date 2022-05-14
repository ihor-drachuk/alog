#pragma once
#include <alog/record.h>
#include <unordered_set>

template<typename... Args>
struct ALog::Internal::is_container<std::unordered_set<Args...>> : public std::true_type { };
