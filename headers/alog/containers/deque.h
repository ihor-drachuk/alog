/* License:  MIT
 * Source:   https://github.com/ihor-drachuk/alog
 * Contact:  ihor-drachuk-libs@pm.me  */

#pragma once
#include <alog/record.h>
#include <deque>

template<typename... Args>
struct ALog::Internal::is_container<std::deque<Args...>> : public std::true_type { };
