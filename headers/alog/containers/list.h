#pragma once
#include <alog/record.h>
#include <list>

template<typename... Args>
struct ALog::Internal::is_container<std::list<Args...>> : public std::true_type { };
