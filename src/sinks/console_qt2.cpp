#include <alog/sinks/console_qt2.h>

#ifdef ALOG_OS_WINDOWS
#ifdef ALOG_HAS_QT_LIBRARY

#include <QDebug>
#include <alog/adapters/qt_qml_adapter.h>

namespace ALog {
namespace Sinks {

void ConsoleQt2::write(const Buffer& buffer, const Record& record)
{
    if (buffer.size() == 0)
        return;

    auto backHandler = QtQmlAdapter::exists() ? QtQmlAdapter::instance()->getBackHandler() : nullptr;

    if (backHandler) {
        QtMsgType type;
        QMessageLogContext ctx;

        switch (record.severity) {
            case ALog::Severity::Verbose: type = QtDebugMsg;    break;
            case ALog::Severity::Debug:   type = QtDebugMsg;    break;
            case ALog::Severity::Info:    type = QtInfoMsg;     break;
            case ALog::Severity::Warning: type = QtWarningMsg;  break;
            case ALog::Severity::Error:   type = QtCriticalMsg; break;
            case ALog::Severity::Fatal:   type = QtFatalMsg;    break;
            default:
                assert(!"Unexpected 'record.severity'!");
        }

        //ctx.file = record.filenameOnly;
        //ctx.line = record.line;
        //ctx.category = record.module;
        //ctx.function = record.func;

        backHandler(type, ctx, QString::fromUtf8((const char*)buffer.data(), static_cast<int>(buffer.size())));

    } else {
        QDebug (QMessageLogger::*logFunc)() const = nullptr;

        switch (record.severity) {
            case ALog::Severity::Verbose: logFunc = &QMessageLogger::debug;    break;
            case ALog::Severity::Debug:   logFunc = &QMessageLogger::debug;    break;
            case ALog::Severity::Info:    logFunc = &QMessageLogger::info;     break;
            case ALog::Severity::Warning: logFunc = &QMessageLogger::warning;  break;
            case ALog::Severity::Error:   logFunc = &QMessageLogger::critical; break;
            case ALog::Severity::Fatal:   logFunc = &QMessageLogger::critical; break;
        }

        assert(logFunc);

        QMessageLogger qtLogger(record.filenameFull, record.line, record.func, nullptr);
        (qtLogger.*logFunc)().noquote() << QString::fromUtf8((const char*)buffer.data(), static_cast<int>(buffer.size()));
    }
}

} // namespace Sinks
} // namespace ALog

#endif // ALOG_HAS_QT_LIBRARY
#endif // ALOG_OS_WINDOWS
