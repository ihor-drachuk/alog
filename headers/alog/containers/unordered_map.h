#pragma once
#include <alog/record.h>
#include <unordered_map>

template<typename... Args>
struct ALog::Internal::is_container<std::unordered_map<Args...>> : public std::true_type { };

template<typename... Args>
struct ALog::Internal::has_key<std::unordered_map<Args...>> : public std::true_type { };
