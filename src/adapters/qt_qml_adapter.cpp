/* License:  MIT
 * Source:   https://github.com/ihor-drachuk/alog
 * Contact:  ihor-drachuk-libs@pm.me  */

#include <alog/adapters/qt_qml_adapter.h>

#ifdef ALOG_HAS_QT_LIBRARY

#define ALOGGER_PREFIX thisPtr->impl().
#include <QString>
#include <alog/logger.h>

namespace ALog {

struct QtQmlAdapter::impl_t
{
    DEFINE_ALOGGER_MODULE(QtQmlAdapter);

    bool forwardToNative;
    Handler oldHandler;
};


QtQmlAdapter::QtQmlAdapter(bool forwardToNative)
{
    createImpl();
    impl().forwardToNative = forwardToNative;
    impl().oldHandler = qInstallMessageHandler(messageOutput);
}

QtQmlAdapter::~QtQmlAdapter()
{
    qInstallMessageHandler(impl().oldHandler);
}

QtQmlAdapter::Handler QtQmlAdapter::getBackHandler() const
{
    return impl().oldHandler ? impl().oldHandler : nullptr;
}

void QtQmlAdapter::messageOutput(QtMsgType type, const QMessageLogContext& context, const QString& msg)
{
    auto thisPtr = QtQmlAdapter::instance();

    Severity severity;
    switch (type) {
        case QtDebugMsg:    severity = Severity::Debug;   break;
        case QtInfoMsg:     severity = Severity::Info;    break;
        case QtWarningMsg:  severity = Severity::Warning; break;
        case QtCriticalMsg: severity = Severity::Error;   break;
        case QtFatalMsg:    severity = Severity::Fatal;   break;
        default:
            assert(false && "Unexpected type in `messageOutput`!");
    }

    static const char* const emptyString = "";

    auto record = Record::create(severity,
                                 context.line,
                                 context.file ? context.file : emptyString,
                                 context.file ? ALog::Internal::extractFileNameOnly(context.file) : emptyString,
                                 context.function ? context.function : emptyString);
    record.module = context.category ? context.category : emptyString;

    record.flagsOn(Record::Flags::Flush);

    auto msgUtf8 = msg.toUtf8();
    if (msgUtf8.size() > 0) {
        assert(*msgUtf8.rbegin() != 0);
        record.message.appendString(msgUtf8.constData(), msgUtf8.size());
    } else {
        record.message.appendString("<Empty input passed to ALog>");
    }

    auto logger = ALog::LoggerHolder<0>::instance()->get();
    static bool hasDebt = false;

    if (logger) {
        if (hasDebt) {
            hasDebt = false;
            ACCESS_ALOGGER_MODULE.flush();
        }

        logger->addRecord(std::move(record));
    } else {
        hasDebt = true;
        LOGE << "Can't send log message!";
    }

    if (thisPtr->impl().forwardToNative && thisPtr->impl().oldHandler) {
        thisPtr->impl().oldHandler(type, context, msg);
    }
}

} // namespace ALog

#endif // ALOG_HAS_QT_LIBRARY
