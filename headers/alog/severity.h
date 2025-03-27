/* License:  MIT
 * Source:   https://github.com/ihor-drachuk/alog
 * Contact:  ihor-drachuk-libs@pm.me  */

#pragma once

namespace ALog {

enum Severity {
    Minimal,
    Verbose = Minimal,
    Debug,
    Info,
    Warning,
    Error,
    Fatal,
    Maximal = Fatal,

    COUNT
};

} // namespace ALog
