#pragma once
#include <alog/record.h>
#include <forward_list>

template<typename... Args>
struct ALog::Internal::is_container<std::forward_list<Args...>> : public std::true_type { };
