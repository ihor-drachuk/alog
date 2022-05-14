#pragma once
#include <alog/record.h>
#include <map>

template<typename... Args>
struct ALog::Internal::is_container<std::multimap<Args...>> : public std::true_type { };

template<typename... Args>
struct ALog::Internal::has_key<std::multimap<Args...>> : public std::true_type { };
