#pragma once

#ifdef ALOG_HAS_QT_LIBRARY
#include <QtGlobal>
#include <alog/tools.h>

namespace ALog {

class QtQmlAdapter : public ALog::Internal::Singleton<QtQmlAdapter>
{
    ALOG_NO_COPY_MOVE(QtQmlAdapter);
#ifdef ALOG_WINDOWS
    friend class ConsoleQt2;
#endif
public:
    QtQmlAdapter(bool forwardToNative = false);
    ~QtQmlAdapter();

private:
    using Handler = decltype(qInstallMessageHandler(nullptr));

    Handler getBackHandler() const;
    static void messageOutput(QtMsgType type, const QMessageLogContext& context, const QString& msg);

private:
    ALOG_DECLARE_PIMPL
};

} // namespace ALog

#endif // ALOG_HAS_QT_LIBRARY
