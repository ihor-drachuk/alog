## ALog - fast simple flexible C++ logger

[![Build & test](https://github.com/ihor-drachuk/alog/actions/workflows/ci.yml/badge.svg?branch=master)](https://github.com/ihor-drachuk/alog/actions/workflows/ci.yml)

### Table of contents
   * [ALog - fast simple flexible C++ logger](#alog---fast-simple-flexible-c-logger)
      * [Requirements](#requirements)
      * [Features](#features)
         * [Currently implemented sinks, formatters &amp; filters](#currently-implemented-sinks-formatters--filters)
         * [Currently supported log-flags](#currently-supported-log-flags)
   * [Setup (CMake)](#setup-cmake)
      * [Option #1: auto-download](#option-1-auto-download)
      * [Option #2: manual download](#option-2-manual-download)
   * [Examples](#examples)
      * [#1. Intro](#1-intro)
      * [#2. Module title - recommended](#2-module-title---recommended)
      * More examples TBD...

### Requirements
- CMake, C++17 compiler
- Windows / Linux / Mac
- No dependencies

### Features

- Fast - 150 ns (0.00015 ms) per record
  - \+ optional asynchronous mode
- Informative - each record contains
  - Several time marks, severity, message
  - File, function, line
  - Module name, thread ID, thread name
- Functional
  - Log anything: containers, pointers, raw buffers, custom types
  - Operates on UTF-8 encoding - log any unicode character
  - Conditional logging
    - `LOGE_IF(errorFlag) << "Error!";`
  - Flags. Call `std::abort` or throw exception right from log-record!
    - `LOGE_IF(errorFlag) << "Error!" << THROW;`
  - Asserts
    - `LOG_ASSERT`, `LOG_ASSERT_D`, `LOG_ASSERT_THROW`. Example: `LOG_ASSERT(errorFlag) << "Error!";`
  - Auto-quotes
  - Auto-separators
- Flexible & configurable
  - Chain of sinks supported
  - Filters
    - Filter by log level, sink, module, thread or whatever you want
    - Chains of filters. Logic combinations of filters (AND, OR, ...)
    - Custom implemented filters
  - Synchronous or asynchronous mode
  - Optional logs sorting in async mode
  - Optional auto-flush for each record
- Extendable interfaces
  - ISink -- implement custom sink (...maybe send logs over network?)
  - IFilter -- if you need special logs-filtering logic (...remove sensitive information?)
  - IFormatter -- decide how & which fields of record to print (another dimension of verbosity)
  - IConverter -- change encoding, line endings, compress, encrypt...

#### Currently implemented sinks, formatters & filters
 - Sinks
   - Standard stream <sub><sup>(SinkStdStream)</sup></sub>
   - File <sub><sup>(SinkSimpleFile)</sup></sub>
   - Null <sub><sup>(SinkNull)</sup></sub>
   - Functor / Lambda <sub><sup>(SinkFunctor)</sup></sub>
   - Virtual sink with filter + nested sink <sub><sup>(SinkWithFilter)</sup></sub>
   - Virtual sink with multiple nested sinks <sub><sup>(SinkContainer)</sup></sub>
 - Formatters
   - Default formatter
   - Minimal formatter (text-only)
 - Filters
   - By severity <sub><sup>(FilterSeverity)</sup></sub>
   - By module & severity <sub><sup>(FilterModuleSeverity)</sup></sub>
   - By nested filters set <sub><sup>(FilterStorage)</sup></sub>

#### Currently supported log-flags
 - `FLUSH`, `THROW`, `ABORT`
 - `NO_AUTO_QUOTES`, `PREFER_QUOTES`
 - `SEPARATOR`, `SEPARATOR_ONCE`, `NO_SEPARATOR`, `SEPARATOR_FORCE`, `SEPARATOR_FORCE_ONCE`

More extensions for ALog implemented separately: [alog-extensions](https://github.com/ihor-drachuk/alog-extensions)

---

## Setup (CMake)

### Option #1: auto-download
- `git` should be installed
- Place these files in your project folder: [ALog-download.txt](https://raw.githubusercontent.com/ihor-drachuk/alog/cmake-autodownload/ALog-download.txt), [ALog-download.in](https://raw.githubusercontent.com/ihor-drachuk/alog/cmake-autodownload/ALog-download.in)
- Add to your CMakeLists:
```CMake
include(ALog-download.txt)
target_link_libraries(YourProject PRIVATE alog)
```

### Option #2: manual download
- Clone ALog manually
- Add to your CMakeLists:
```CMake
add_subdirectory(3rd-party/alog)
target_link_libraries(YourProject PRIVATE alog)
```
---

## Examples

### #1. Intro

**Source**
```C++
#include <alog/logger.h>


int main() {
    SIMPLE_SETUP_ALOG;

    LOGMD << "Test!";
    LOGMW_IF(1 != 2) << "Another message";
    LOGMI << "Another container: " << std::vector{"str1", "str2"};

    return 0;
}
```

**Output:**
```
[    0.000] T#0  [Debug   ] [::main:7]  Test!
[    0.000] T#0  [Warn    ] [::main:8]  Another message
[    0.000] T#0  [Info    ] [::main:9]  Another container: {Container; Size: 2; Data = "str1", "str2"}
```
- `LOGMD` instead of `LOGD` when module name is not provided. See next example.
- `[    0.000]` - By default, time is specified in [sec.msec] from app start.
- `T#0` - Enumerated threads like 'T#0', 'T#1' much easier to understand than comparing IDs like 71041, 9163, 91273 between themselves, which also changed on each restart.

### #2. Module title - recommended
Source:
```C++
#include <alog/logger.h>
DEFINE_ALOGGER_MODULE(Satellite_Main_Loop);

int main() {
    SIMPLE_SETUP_ALOG;

    LOGD << "Test!";
    LOGW_IF(1 != 2) << "Another message";
    LOGI << "Another container: " << std::vector{"str1", "str2"};

    return 0;
}
```

Output:
```
[    0.000] T#0  [Debug   ] [Satellite_Main_Loop  ] [::main:7]  Test!
[    0.000] T#0  [Warn    ] [Satellite_Main_Loop  ] [::main:8]  Another message
[    0.000] T#0  [Info    ] [Satellite_Main_Loop  ] [::main:9]  Another container: {Container; Size: 2; Data = "str1", "str2"}
```
- Notice! When module title is provided, instead of `LOGMD` we use `LOGD` (recommended).

### More examples TBD...
