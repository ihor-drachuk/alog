<p align="center">
  <h1 align="center">ALog</h1>
  <p align="center">
    <strong>High-Performance C++ Logging Library</strong>
  </p>
  <p align="center">
    Ultra-fast, flexible, and feature-rich logging for modern C++ applications
  </p>
</p>

<p align="center">
  <a href="https://github.com/ihor-drachuk/alog/actions/workflows/ci.yml"><img src="https://github.com/ihor-drachuk/alog/actions/workflows/ci.yml/badge.svg?branch=master" alt="Build & Test"></a>
  <a href="https://github.com/ihor-drachuk/alog/blob/master/License.txt"><img src="https://img.shields.io/badge/License-MIT-blue.svg" alt="License: MIT"></a>
  <img src="https://img.shields.io/badge/C%2B%2B-17%20|%2020%20|%2023-blue.svg" alt="C++ Standard">
  <img src="https://img.shields.io/badge/Platform-Windows%20|%20Linux%20|%20macOS-blueviolet.svg" alt="Platform">
  <img src="https://img.shields.io/badge/Dependencies-None-green.svg" alt="No Dependencies">
</p>

---

## Table of Contents

- [Performance](#performance)
- [Key Features](#key-features)
- [Quick Start](#quick-start)
  - [Installation (CMake)](#installation-cmake)
  - [Basic Usage](#basic-usage)
  - [With Module Names](#with-module-names)
- [Architecture](#architecture)
- [Advanced Usage](#advanced-usage)
  - [Multiple Sinks (Console + File)](#multiple-sinks-console--file)
  - [Filter by Severity](#filter-by-severity)
  - [Advanced Filter Chain](#advanced-filter-chain)
  - [Route Logs to Different Outputs](#route-logs-to-different-outputs)
- [Logging Macros](#logging-macros)
- [Supported Types](#supported-types)
- [Configuration Options](#configuration-options)
- [Extending ALog](#extending-alog)
- [Requirements](#requirements)
- [License](#license)

---

## Performance

| Metric | Value |
|--------|-------|
| **Throughput** | **85 ns** per log record |
| **Records per ms** | ~11,700 |
| **Async mode** | Non-blocking with optional sorting |

---

## Key Features

### Speed
- **85 nanoseconds** per log record — one of the fastest C++ loggers available
- Asynchronous mode with optional chronological sorting
- Zero external dependencies for maximum performance

### Rich Information
Every log record automatically captures:
- Timestamps (elapsed time from application start)
- Severity level (Verbose, Debug, Info, Warning, Error, Fatal)
- Source location (file, function, line)
- Module name, Thread ID, Thread name

### Developer Experience
```cpp
LOGD << "Simple logging";
LOGE_IF(errorFlag) << "Conditional error!";
LOGI << "Container: " << std::vector{1, 2, 3};
LOGW << "Error!" << THROW;  // Log and throw exception
```

### Flexibility
- **Sinks**: Console, File, FileRotated, Baical, Pipeline, Custom
- **Filters**: By severity, module, file, or custom logic
- **Formatters**: Default (full) or Minimal (text-only)
- **Converters**: Encoding, compression, encryption

---

## Quick Start

### Installation (CMake)

**Option 1: FetchContent (Recommended)**
```cmake
include(FetchContent)
FetchContent_Declare(alog
  GIT_REPOSITORY https://github.com/ihor-drachuk/alog.git
  GIT_TAG        master
)
FetchContent_MakeAvailable(alog)

target_link_libraries(YourProject PRIVATE alog)
```

**Option 2: Submodule**
```cmake
add_subdirectory(3rd-party/alog)
target_link_libraries(YourProject PRIVATE alog)
```

### Basic Usage

```cpp
#include <alog/logger.h>

int main() {
    SIMPLE_SETUP_ALOG;  // One-line setup

    LOGMD << "Debug message";
    LOGMI << "Info message";
    LOGMW << "Warning message";
    LOGME << "Error message";

    return 0;
}
```

**Output:**
```
[    0.000] T#0  [Debug   ] [::main:5]  Debug message
[    0.000] T#0  [Info    ] [::main:6]  Info message
[    0.000] T#0  [Warn    ] [::main:7]  Warning message
[    0.000] T#0  [Error   ] [::main:8]  Error message
```

### With Module Names

```cpp
#include <alog/logger.h>
DEFINE_ALOGGER_MODULE_NS(NetworkManager);

void connect() {
    LOGD << "Connecting to server...";
    LOGI << "Connection established";
}
```

**Output:**
```
[    0.001] T#0  [Debug   ] [NetworkManager       ] [::connect:5]  Connecting to server...
[    0.002] T#0  [Info    ] [NetworkManager       ] [::connect:6]  Connection established
```

---

## Architecture

```
                                            ALog Pipeline
┌────────────────────────────────────────────────────────────────────────────────────────────────┐
│                                                                                                │
│  ┌───────────┐   ┌────────┐   ┌───────────┐   ┌─────────────┐   ┌───────────┐   ┌───────────┐  │
│  │ LOG Macro │──►│ Record │──►│ Formatter │──►│ Converter(s)│──►│ Filter(s) │──►│   Sinks   │  │
│  └───────────┘   └────────┘   └─────┬─────┘   └──────┬──────┘   └─────┬─────┘   └─────┬─────┘  │
│                                     │                │                │               │        │
│                                     ▼                ▼                ▼               ▼        │
│                               ┌───────────┐    ┌───────────┐    ┌───────────┐   ┌───────────┐  │
│                               │  Default  │    │ Attention │    │ Severity  │   │  Console  │  │
│                               │  Minimal  │    └───────────┘    │  Module   │   │    File   │  │
│                               └───────────┘                     │   File    │   │FileRotated│  │
│                                                                 │    ...    │   │  Pipeline │  │
│                                                                 └───────────┘   │    ...    │  │
│                                                                                 └───────────┘  │
│                                                                                                │
└────────────────────────────────────────────────────────────────────────────────────────────────┘
```

### Components

| Component | Purpose | Examples |
|-----------|---------|----------|
| **Sinks** | Output destinations | Console, File, FileRotated, Baical, Pipeline |
| **Filters** | Control which logs pass | Severity, Module, File, Chain, Custom |
| **Formatters** | Format log output | Default (full info), Minimal (text only) |
| **Converters** | Post-process output | Encoding, line endings, compression |

---

## Advanced Usage

### Multiple Sinks (Console + File)

```cpp
#include <alog/all.h>
DEFINE_ALOGGER_MODULE_NS(Main);

int main() {
    ALog::DefaultLogger logger;
    logger->setMode(ALog::Logger::LoggerMode::AsynchronousSort);
    logger->pipeline().sinks().add(
        std::make_shared<ALog::Sinks::File>("app.log")
    );
    logger.markReady();

    LOGD << "This goes to both console and file";
    return 0;
}
```

### Filter by Severity

```cpp
// Only log Warning and above
logger->pipeline().filters().set(
    std::make_shared<ALog::Filters::Severity>(ALog::Severity::Warning)
);
```

### Advanced Filter Chain

```cpp
auto filters = ALog::Filters::Chain::create({
    // Always pass Warning+
    std::make_shared<ALog::Filters::Severity>(
        ALog::Severity::Warning,
        ALog::IFilter::PassOrUndefined
    ),
    // Exclude noisy module
    std::make_shared<ALog::Filters::Module>(
        "NoisyModule", false,
        ALog::IFilter::RejectOrUndefined
    ),
    // Default: reject
    std::make_shared<ALog::Filters::Always>(false)
});
logger->pipeline().filters().set(filters);
```

### Route Logs to Different Outputs

```cpp
// Warnings+ → stderr, others → stdout
auto pipelineStdout = std::make_shared<ALog::Sinks::Pipeline>();
pipelineStdout->filters().set(
    std::make_shared<ALog::Filters::Severity>(
        ALog::Severity::Warning, ALog::IFilter::Default, ALog::Less
    )
);
pipelineStdout->sinks().set(
    std::make_shared<ALog::Sinks::Console>(ALog::Sinks::Console::Stream::StdOut)
);

auto pipelineStderr = std::make_shared<ALog::Sinks::Pipeline>();
pipelineStderr->filters().set(
    std::make_shared<ALog::Filters::Severity>(
        ALog::Severity::Warning, ALog::IFilter::Default, ALog::GreaterEqual
    )
);
pipelineStderr->sinks().set(
    std::make_shared<ALog::Sinks::Console>(ALog::Sinks::Console::Stream::StdErr)
);

logger->pipeline().sinks().set({pipelineStdout, pipelineStderr});
```

---

## Logging Macros

### Basic Macros

| Macro | Description |
|-------|-------------|
| `LOGV` / `LOGMV` | Verbose |
| `LOGD` / `LOGMD` | Debug |
| `LOGI` / `LOGMI` | Info |
| `LOGW` / `LOGMW` | Warning |
| `LOGE` / `LOGME` | Error |
| `LOGF` / `LOGMF` | Fatal |

> **Note:** Use `LOG*` when module is defined via `DEFINE_ALOGGER_MODULE_NS`, use `LOGM*` otherwise.

### Conditional Logging

```cpp
LOGD_IF(condition) << "Only logged if condition is true";
LOGE_IF(error) << "Error occurred!";
```

### Log Flags

| Flag | Description |
|------|-------------|
| `FLUSH` | Force immediate flush |
| `THROW` | Log message and throw exception |
| `ABORT` | Log message and call `std::abort()` |
| `BUFFER(ptr, size)` | Log raw binary buffer |
| `SEPARATORS` / `NSEPS` | Enable/disable auto-separators |
| `AUTO_QUOTES` / `NO_AUTO_QUOTES` | Enable/disable auto-quoting |

```cpp
LOGE << "Critical failure!" << ABORT;
LOGD << "Buffer content: " << BUFFER(data, size);
```

### Assertions

```cpp
LOG_ASSERT(condition) << "Assertion failed info";
LOG_ASSERT_D(condition) << "Debug-only assertion";
LOG_ASSERT_THROW(condition) << "Throws on failure";
```

---

## Supported Types

ALog automatically formats common types:

- **STL Containers**: `vector`, `map`, `set`, `list`, `deque`, etc. (requires `<alog/containers/all.h>`)
- **Smart Pointers**: `unique_ptr`, `shared_ptr`
- **Optionals**: `std::optional`, `std::expected`
- **Qt Types** (when Qt available): `QString`, `QPoint`, `QJsonObject`, etc.
- **Raw Buffers**: Via `BUFFER(ptr, size)` flag
- **Custom Types**: Implement `operator<<`

```cpp
#include <alog/containers/all.h>  // Required for container support

LOGI << std::vector{1, 2, 3};
// Output: {Container; Size: 3; Data = 1, 2, 3}

LOGI << std::map<std::string, int>{{"a", 1}, {"b", 2}};
// Output: {Container; Size: 2; Data = ("a", 1), ("b", 2)}
```

---

## Configuration Options

### Logger Modes

| Mode | Description |
|------|-------------|
| `Synchronous` | Blocking, immediate output (best for debugging) |
| `Asynchronous` | Non-blocking, background processing |
| `AsynchronousSort` | Non-blocking with chronological sorting |

```cpp
logger->setMode(ALog::Logger::LoggerMode::AsynchronousSort);
logger->setAutoflush(false);  // Batch writes for performance
```

### CMake Options

| Option | Default | Description |
|--------|---------|-------------|
| `ALOG_ENABLE_TESTS` | OFF | Build test suite |
| `ALOG_ENABLE_BENCHMARK` | OFF | Build benchmarks |
| `ALOG_CXX_STANDARD` | 17 | C++ standard (17, 20, 23) |
| `ALOG_ENABLE_DEF_SEPARATORS` | OFF | Auto-separators between values |
| `ALOG_ENABLE_DEF_AUTO_QUOTES` | ON | Auto-quote strings |

---

## Extending ALog

### Custom Sink

```cpp
class MySink : public ALog::ISink {
public:
    void write(const ALog::Buffer& buffer, const ALog::Record& record) override {
        // Send to your destination
    }
    void flush() override { }
};

logger->pipeline().sinks().add(std::make_shared<MySink>());
```

### Custom Filter

```cpp
class MyFilter : public ALog::IFilter {
protected:
    ALog::I::optional_bool canPassImpl(const ALog::Record& record) const override {
        // Return true/false/undefined
        return record.severity >= ALog::Severity::Warning;
    }
};
```

---

## Requirements

- **Compiler**: C++17 or later (GCC, Clang, MSVC, Apple Clang)
- **Build System**: CMake 3.16+
- **Platforms**: Windows, Linux, macOS
- **Dependencies**: None (Qt integration optional)

---

## License

MIT License — see [License.txt](License.txt) for details.

Copyright (c) 2018-2026 Ihor Drachuk

---

## Author

**Ihor Drachuk** — [ihor-drachuk-libs@pm.me](mailto:ihor-drachuk-libs@pm.me)

[GitHub](https://github.com/ihor-drachuk/alog)
