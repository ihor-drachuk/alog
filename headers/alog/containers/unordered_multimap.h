/* License:  MIT
 * Source:   https://github.com/ihor-drachuk/alog
 * Contact:  ihor-drachuk-libs@pm.me  */

#pragma once
#include <alog/record.h>
#include <unordered_map>

template<typename... Args>
struct ALog::Internal::is_container<std::unordered_multimap<Args...>> : public std::true_type { };

template<typename... Args>
struct ALog::Internal::has_key<std::unordered_multimap<Args...>> : public std::true_type { };
