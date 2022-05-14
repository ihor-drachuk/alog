#pragma once
#include <alog/record.h>
#include <set>

template<typename... Args>
struct ALog::Internal::is_container<std::multiset<Args...>> : public std::true_type { };
