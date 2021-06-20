## ALog - fast simple flexible C++ logger

[![Build & test](https://github.com/ihor-drachuk/alog/actions/workflows/ci.yml/badge.svg?branch=master)](https://github.com/ihor-drachuk/alog/actions/workflows/ci.yml)

### Table of contents
   * [ALog - fast simple flexible C++ logger](#alog---fast-simple-flexible-c-logger)
      * [Table of contents](#table-of-contents)
      * [Requirements](#requirements)
      * [Features](#features)
         * [Currently implemented sinks, formatters &amp; filters](#currently-implemented-sinks-formatters--filters)
         * [Currently supported log-flags](#currently-supported-log-flags)
   * [Setup (CMake)](#setup-cmake)
      * [Option #1: auto-download](#option-1-auto-download)
      * [Option #2: manual download](#option-2-manual-download)
   * [Examples](#examples)
      * [#1. Intro](#1-intro)
      * [#2. Declare module title](#2-declare-module-title)
      * [#3. Extended (non-simple) logger setup](#3-extended-non-simple-logger-setup)
      * [#4. Duplicate logs to file](#4-duplicate-logs-to-file)
      * [#5. Filter out non-important logs](#5-filter-out-non-important-logs)
      * [#6. Advanced I - Part 1: Filters chain](#6-advanced-i---part-1-filters-chain)
      * [#7. Advanced I - Part 2: Understanding of RejectOrUndefined](#7-advanced-i---part-2-understanding-of-rejectorundefined)
      * [#8. Advanced II: different filters for different sinks](#8-advanced-ii-different-filters-for-different-sinks)
      * [More examples TBD...](#more-examples-tbd)

_<sup>(Thanks to [ekalinin/github-markdown-toc](https://github.com/ekalinin/github-markdown-toc))</sup>_

### Requirements
- CMake, C++17 compiler
- Windows / Linux / Mac
- No dependencies

### Features

- Ultra fast - 160 ns (0.00016 ms) per record. It's 6250 records per ms!
  - \+ optional asynchronous mode
- Informative - each record contains
  - Several time marks, severity, message
  - File, function, line
  - Module name, thread ID, thread name
- Functional
  - Log anything: containers, pointers, raw buffers, custom types
  - Based on UTF-8 encoding - log any unicode character
  - Qt-friendly
  - Conditional logging
    - `LOGE_IF(errorFlag) << "Error!";`
  - Flags. Call `std::abort` or throw exception right from log-record!
    - `LOGE_IF(errorFlag) << "Error!" << THROW;`
  - Asserts
    - `LOG_ASSERT`, `LOG_ASSERT_D`, `LOG_ASSERT_THROW`. Example: `LOG_ASSERT(errorFlag) << "Error info";`
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
   - Console
   - ConsoleUTF8
   - File
   - ...special: Null, Functor, Chain, Pipeline
 - Formatters:
   - Default formatter
   - Minimal formatter (text-only)
 - Filters
   - By module
   - By source file
   - By severity
   - By severity & module
   - By severity & file
   - By tag: sensitive information, low-level IO dumps, etc.
   - ...special: Always, Functor, Chain

#### Currently supported log-flags
 - `BUFFER` -- for printing RAW buffers
 - `FLUSH`
 - `THROW`, `ABORT`
 - `SEPARATORS` (`SEPS`), `NO_SEPARATORS` (`NSEPS`), `SEP(new_separator)`, `SSEP(skip_count)`
 - `AUTO_QUOTES`, `NO_AUTO_QUOTES`, `QUOTE_LITERALS`

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
- `LOGMD` used instead of `LOGD` when module name is not provided.
- `[    0.000]` - By default, time is specified in [sec.msec] from app start.
- `T#0` - Enumerated threads like 'T#0', 'T#1' much easier to understand than comparing IDs like 71041, 9163, 91273 between themselves, which also changed on each restart.

### #2. Declare module title
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
- Notice! When module title is provided, instead of `LOGMD` we use `LOGD`.
- It's recommended to provide module name always

### #3. Extended (non-simple) logger setup
```C++
#include <alog/logger.h>
DEFINE_ALOGGER_MODULE(Main);

int main() {
    ALog::DefaultLogger logger;                                    //      During trace debug recommended to set:
    logger->setMode(ALog::Logger::LoggerMode::AsynchronousSort);   // <--   - Synchronous
    logger->setAutoflush(false);                                   // <--   - true
    logger->pipeline().sinks().set( ... );
    logger->pipeline().filters().set( ... );
    logger->pipeline().converters().set( ... );
    // Don't log anything before call to `markReady`!
    logger.markReady();
    // Don't change logger settings after call to `markReady`!

    LOGD << "Test!";

    return 0;
}
```

### #4. Duplicate logs to file
```C++
#include <alog/all.h>
DEFINE_ALOGGER_MODULE(Main);

int main() {
    ALog::DefaultLogger logger;
    logger->setMode(ALog::Logger::LoggerMode::AsynchronousSort);
    logger->setAutoflush(false);
    // By default logger contains sink `Console`. Now we're adding another one: `File`.
    logger->pipeline().sinks().add( std::make_shared<ALog::Sinks::File>("logs.txt") );
    logger.markReady();

    LOGD << "Test!";

    return 0;
}
```


### #5. Filter out non-important logs
```C++
#include <alog/all.h>
DEFINE_ALOGGER_MODULE(Main);

int main() {
    ALog::DefaultLogger logger;
    logger->setMode(ALog::Logger::LoggerMode::AsynchronousSort);
    logger->setAutoflush(false);
    // Pass logs with severity "Warning" and higher
    logger->pipeline().filters().set( std::make_shared<ALog::Filters::Severity>(ALog::Severity::Warning) );
    logger.markReady();

    LOGD << "Test!";

    return 0;
}
```

### #6. Advanced I - Part 1: Filters chain
```C++
#include <alog/all.h>
DEFINE_ALOGGER_MODULE(Main);

int main() {
    auto filters = ALog::Filters::Chain::create({
        // Always pass 'Warning' and higher
        std::make_shared<ALog::Filters::Severity>(ALog::Severity::Warning, ALog::IFilter::PassOrUndefined),

        // Always exclude logs from MyNoisyModule
        std::make_shared<ALog::Filters::Module>("MyNoisyModule", false, ALog::IFilter::RejectOrUndefined),

        // Always pass all logs from MyUnstableModule
        std::make_shared<ALog::Filters::Module>("MyUnstableModule", true, ALog::IFilter::PassOrUndefined),

        // Temporary uncomment line below to quickly enable all logs (except MyNoisyModule, because it's processed/triggered earlier in chain)
        // std::make_shared<ALog::Filters::Always>(true),

        // Explicit default decision (reject) if none of the rules above triggered
        std::make_shared<ALog::Filters::Always>(false)
    });

    ALog::DefaultLogger logger;
    logger->setMode(ALog::Logger::LoggerMode::AsynchronousSort);
    logger->setAutoflush(false);
    logger->pipeline().filters().set(filters);
    logger.markReady();

    LOGD << "Test!";

    return 0;
}
```

### #7. Advanced I - Part 2: Understanding of `RejectOrUndefined`

`ALog::Filters::Chain` is container of other filters.

When log record is tested by the `Chain` filter, it's internally tested by each nested filter until one of them will answer `true` (accept) or `false` (reject). Filters also can answer `Undefined`, it means that current log record is not accepted nor rejected by this filter, so it will be tested by the next one.

Ok. Each filter can 'accept', 'reject' and 'hold the answer'. So, what means special filter mode `RejectOrUndefined` and what other modes exist?

There are 3 modes:
 - PassOrReject (default). When some strict binary filter answers `true` or `false`, log record is passed or dropped respectively. It's not possible to apply some additional (2nd, 3rd...) filter in chain for log record because decision is always determined and applied by the first filter.
 - PassOrUndefined. Binary filter logic `true/false` is converted to `true/undefined`. Filter can accept record, or do nothing. Useful, when you definitely need to pass log record by some condition, but other logs are not affected by this filter and will be tested by next filters in chain.
 - RejectOrUndefined. Binary filter logic `true/false` is converted to `undefined/false`. Filter can drop record, or do nothing. Useful, when you definitely need to reject log record by some condition, but other logs are not affected by this filter and will be tested by next filters in chain.

### #8. Advanced II: different filters for different sinks
```C++
#include <alog/all.h>
DEFINE_ALOGGER_MODULE(Main);

int main() {
    // If less than 'Warning' ==> stdout
    auto pipelineStdout = std::make_shared<ALog::Sinks::Pipeline>();
    pipelineStdout->filters().set( std::make_shared<ALog::Filters::Severity>(ALog::Severity::Warning, ALog::IFilter::Default, ALog::Less) );
    pipelineStdout->sinks().set( std::make_shared<ALog::Sinks::Console>(ALog::Sinks::Console::Stream::StdOut) );

    // If 'Warning' or higher ==> stderr
    auto pipelineStderr = std::make_shared<ALog::Sinks::Pipeline>();
    pipelineStderr->filters().set( std::make_shared<ALog::Filters::Severity>(ALog::Severity::Warning, ALog::IFilter::Default, ALog::GreaterEqual) );
    pipelineStderr->sinks().set( std::make_shared<ALog::Sinks::Console>(ALog::Sinks::Console::Stream::StdErr) );

    // Also without any conditions and filter write to "all-logs.txt"
    auto pipelineFile = std::make_shared<ALog::Sinks::Pipeline>();
    pipelineFile->filters().set({});
    pipelineFile->sinks().set( std::make_shared<ALog::Sinks::File>("all-logs.txt") );


    ALog::DefaultLogger logger;
    logger->setMode(ALog::Logger::LoggerMode::AsynchronousSort);
    logger->setAutoflush(false);
    logger->pipeline().sinks().set({pipelineStdout, pipelineStderr, pipelineFile});
    logger.markReady();

    LOGD << "Test!";

    return 0;
}
```


### More examples TBD...
