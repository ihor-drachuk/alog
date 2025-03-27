/* License:  MIT
 * Source:   https://github.com/ihor-drachuk/alog
 * Contact:  ihor-drachuk-libs@pm.me  */

#include <alog/sinks/console_qt2.h>

#ifdef ALOG_OS_WINDOWS
#ifdef ALOG_HAS_QT_LIBRARY

#include <QDebug>
#include <alog/adapters/qt_qml_adapter.h>
#include <alog/tools_internal.h>
#include <array>

namespace ALog {
namespace Sinks {

namespace {

using LogFuncPtrType = QDebug (QMessageLogger::*)() const;

struct SeverityData
{
    QtMsgType qtSeverity;
    LogFuncPtrType qtLogFuncPtr;
    QString color;
};

} // namespace

struct ConsoleQt2::impl_t
{
    bool colored {};

    const std::array<SeverityData, ALog::Severity::COUNT> severityMap {
        SeverityData {QtDebugMsg,    &QMessageLogger::debug,    Internal::getSeverityColorCode(Severity::Verbose).c_str()},
        SeverityData {QtDebugMsg,    &QMessageLogger::debug,    Internal::getSeverityColorCode(Severity::Debug).c_str()  },
        SeverityData {QtInfoMsg,     &QMessageLogger::info,     Internal::getSeverityColorCode(Severity::Info).c_str()   },
        SeverityData {QtWarningMsg,  &QMessageLogger::warning,  Internal::getSeverityColorCode(Severity::Warning).c_str()},
        SeverityData {QtCriticalMsg, &QMessageLogger::critical, Internal::getSeverityColorCode(Severity::Error).c_str()  },
        SeverityData {QtFatalMsg,    &QMessageLogger::critical, Internal::getSeverityColorCode(Severity::Fatal).c_str()  }
    };

    const QString resetColorCode {Internal::getResetColorCode().c_str()};
};

ConsoleQt2::ConsoleQt2(ColorMode colorMode)
{
    createImpl();
    impl().colored = (colorMode == ColorMode::Auto && Internal::enableColoredTerminal()) ||
                      colorMode == ColorMode::Force;
}

ConsoleQt2::~ConsoleQt2() = default;

void ConsoleQt2::write(const Buffer& buffer, const Record& record)
{
    if (buffer.empty())
        return;

    const QString& msgInitial = QString::fromUtf8(reinterpret_cast<const char*>(buffer.data()), static_cast<int>(buffer.size()));
    QString msg;
    const auto backHandler = QtQmlAdapter::exists() ? QtQmlAdapter::instance()->getBackHandler() : nullptr;
    const auto& severityData = impl().severityMap[record.severity];

    if (impl().colored) {
        msg.reserve(buffer.size() + 16);
        msg += severityData.color;
        msg += msgInitial;
        msg += impl().resetColorCode;

    } else {
        msg = msgInitial;
    }

    if (backHandler) {
        backHandler(severityData.qtSeverity, {}, msg);

    } else {
        const LogFuncPtrType logFunc = severityData.qtLogFuncPtr;
        const QMessageLogger qtLogger(record.filenameFull, record.line, record.func, nullptr);
        (qtLogger.*logFunc)().noquote() << msg;
    }
}

} // namespace Sinks
} // namespace ALog

#endif // ALOG_HAS_QT_LIBRARY
#endif // ALOG_OS_WINDOWS
