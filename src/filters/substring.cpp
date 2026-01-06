/* License:  MIT
 * Source:   https://github.com/ihor-drachuk/alog
 * Contact:  ihor-drachuk-libs@pm.me  */

#include <alog/filters/substring.h>
#include <algorithm>
#include <cctype>
#include <cstring>

namespace ALog {
namespace Filters {

struct Substring::impl_t
{
    std::string substring; // lowercase, when case-insensitive
    bool pass {};
    bool caseSensitive {};
};

namespace {

bool containsSubstring(const char* haystack, const std::string& needle, bool caseSensitive)
{
    if (!haystack || needle.empty())
        return false;

    if (caseSensitive)
        return strstr(haystack, needle.c_str()) != nullptr;

    const char* haystackEnd = haystack + strlen(haystack);
    auto it = std::search(haystack, haystackEnd,
                          needle.begin(), needle.end(),
                          [](unsigned char a, unsigned char b) {
                              return std::tolower(a) == b;
                          });
    return it != haystackEnd;
}

} // namespace

Substring::Substring(const char* substring, bool pass, bool caseSensitive, Mode mode)
    : IFilter(mode)
{
    createImpl();
    impl().substring = substring;
    impl().pass = pass;
    impl().caseSensitive = caseSensitive;

    if (!caseSensitive) {
        std::transform(impl().substring.begin(), impl().substring.end(),
                       impl().substring.begin(),
                       [](unsigned char c) { return static_cast<char>(std::tolower(c)); });
    }
}

Substring::~Substring() = default;

I::optional_bool Substring::canPassImpl(const Record& record) const
{
    const char* msg = record.getMessage();
    if (!msg)
        return impl().pass;

    bool contains = containsSubstring(msg, impl().substring, impl().caseSensitive);

    return !(contains ^ impl().pass);
}

} // namespace Filters
} // namespace ALog
