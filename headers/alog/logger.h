#pragma once
#include <mutex>
#include <vector>
#include <alog/logger-impl.h>

namespace ALog {

struct MockRecord { };

template<int Number = 0>
class LoggerN : public Logger { };

template<int Number = 0>
using LoggerHolder = SIOS<LoggerN<Number>>;

template<int Number = 0>
class LoggerEntry : protected SIOS_Entry<LoggerN<Number>>
{
public:
    // Warning! Module name is not copied and should be literal!

    LoggerEntry(const char* module = nullptr) {
        m_module = module;
        m_masterAvailable = this->available();
    }

    void flush() {
        *this += Record::create(ALog::Record::Flags::FlushAndDrop);
    }

    void operator+= (Record&& record) {
        record.module = m_module;

        // Lock-free optimization
        if (m_masterAvailable) {
            // Send directly to master
            this->fastGet()->addRecord(std::move(record));
            return;
        }

        std::lock_guard<std::mutex> _lck(m_mutex);

        if (m_masterAvailable) {
            // Send directly to master
            this->fastGet()->addRecord(std::move(record));

        } else {
            // Add record to queue

            Record throwMe (Record::uninitialized_tag{});
            bool throwMeValid { false };
            bool abort { false };

            if (record.flags & (int)Record::Flags::Throw) {
                throwMe = record;
                throwMeValid = true;
            }

            if (record.flags & (int)Record::Flags::Abort) {
                abort = true;
            }

            record.flags |= (int)Record::Flags::Queued;
            m_queue.emplace_back(std::move(record));

            if (this->available()) {
                // Send queue to master
                for (auto& x : m_queue)
                    this->fastGet()->addRecord(std::move(x));
                m_queue.clear();
                m_masterAvailable = true;
            }

            if (abort)
                alog_abort();

            if (throwMeValid)
                alog_exception(throwMe.getMessage(), throwMe.getMessageLen());
        }
    }

private:
    std::mutex m_mutex;
    const char* m_module { nullptr };
    bool m_masterAvailable { false };
    std::vector<Record> m_queue;
};

using DefaultLogger = ALog::LoggerHolder<0>;

} // namespace ALog

template<typename T>
inline ALog::MockRecord&& operator<< (ALog::MockRecord&& r, const T&) { return std::move(r); }

#define DEFINE_MAIN_ALOGGER_N(N)       ALog::LoggerHolder<N> MainALogger_##N
#define DEFINE_MAIN_ALOGGER            DEFINE_MAIN_ALOGGER_N(0)

#define ALOGGER_N(N)                   (*ALog::LoggerHolder<N>::instance()->get())
#define ALOGGER                        ALOGGER_N(0)

#define ALOGGER_DIRECT_N(N)            MainALogger_##N
#define ALOGGER_DIRECT                 ALOGGER_DIRECT_N(0)

#define MARK_ALOGGER_READY_N(N)        ALog::LoggerHolder<N>::instance()->markReady()
#define MARK_ALOGGER_READY             MARK_ALOGGER_READY_N(0)

#define DEFINE_ALOGGER_MODULE_NS_N(N, x)  namespace { ALog::LoggerEntry<N> LoggerEntry_##N{#x}; }
#define DEFINE_ALOGGER_MODULE_NS(x)       DEFINE_ALOGGER_MODULE_NS_N(0, x)

#define DEFINE_ALOGGER_MODULE_N(N, x)  ALog::LoggerEntry<N> LoggerEntry_##N{#x}
#define DEFINE_ALOGGER_MODULE(x)       DEFINE_ALOGGER_MODULE_N(0, x)

#ifdef ALOGGER_PREFIX
#define ACCESS_ALOGGER_MODULE_N(N)     ALOGGER_PREFIX LoggerEntry_##N
#else
#define ACCESS_ALOGGER_MODULE_N(N)     LoggerEntry_##N
#endif
#define ACCESS_ALOGGER_MODULE          ACCESS_ALOGGER_MODULE_N(0)


#define _ALOG_RECORD(Severity)          ALog::Record::create(Severity, __LINE__, __FILE__, ALog::extractFileNameOnly(__FILE__), __func__)
#define _ALOG(Logger, Severity)         Logger += _ALOG_RECORD(Severity)


#define ALOG_MODULE(Severity)            _ALOG(ACCESS_ALOGGER_MODULE, Severity)
#define ALOG_MODULE_N(N, Severity)       _ALOG(ACCESS_ALOGGER_MODULE_N(N), Severity)
#define ALOG_MAIN(Severity)              _ALOG(ALOGGER, Severity)
#define ALOG_MAIN_N(N, Severity)         _ALOG(ALOGGER_N(N), Severity)

#define ALOG_MODULE_IF(Cond, Severity)        if (!(Cond)) {;} else _ALOG(ACCESS_ALOGGER_MODULE, Severity)
#define ALOG_MODULE_IF_N(N, Cond, Severity)   if (!(Cond)) {;} else _ALOG(ACCESS_ALOGGER_MODULE_N(N), Severity)
#define ALOG_MAIN_IF(Cond, Severity)          if (!(Cond)) {;} else _ALOG(ALOGGER, Severity)
#define ALOG_MAIN_IF_N(N, Cond, Severity)     if (!(Cond)) {;} else _ALOG(ALOGGER_N(N), Severity)

// Special
#define ALOG_FL_FLUSH                 ALog::Record::Flags::Flush
#define ALOG_FL_THROW                 ALog::Record::Flags::ThrowSync
#define ALOG_FL_ABORT                 ALog::Record::Flags::AbortSync
#define ALOG_FL_NO_AUTO_QUOTES        ALog::Record::Flags::NoAutoQuote
#define ALOG_FL_SKIP_AUTO_QUOTES      ALog::Record::Flags::SkipAutoQuote
#define ALOG_FL_PREFER_QUOTES         ALog::Record::Flags::PreferAutoQuoteLitStr

#define ALOG_BUFFER(ptr, sz)          ALog::Record::RawData::create(ptr, sz)

#define ALOG_ASSERT(cond)             ALOGF_IF(!(cond)) << ALOG_FL_ABORT << "Assertion failed: " << #cond << ALog::Record::Flags::NeedSeparator
#ifdef NDEBUG
#define ALOG_ASSERT_D(cond)           ALog::MockRecord()
#else
#define ALOG_ASSERT_D(cond)           ALOG_ASSERT(cond)
#endif
#define ALOG_ASSERT_THROW(cond)       ALOGE_IF(!(cond)) << ALOG_FL_THROW << "Exception. Assertion failed: " << #cond << ALog::Record::Flags::NeedSeparator

// Main macros
#define ALOGV                         ALOG_MODULE(ALog::Severity::Verbose)
#define ALOGD                         ALOG_MODULE(ALog::Severity::Debug)
#define ALOGI                         ALOG_MODULE(ALog::Severity::Info)
#define ALOGW                         ALOG_MODULE(ALog::Severity::Warning)
#define ALOGE                         ALOG_MODULE(ALog::Severity::Error)
#define ALOGF                         ALOG_MODULE(ALog::Severity::Fatal)

#define ALOGMV                        ALOG_MAIN(ALog::Severity::Verbose)
#define ALOGMD                        ALOG_MAIN(ALog::Severity::Debug)
#define ALOGMI                        ALOG_MAIN(ALog::Severity::Info)
#define ALOGMW                        ALOG_MAIN(ALog::Severity::Warning)
#define ALOGME                        ALOG_MAIN(ALog::Severity::Error)
#define ALOGMF                        ALOG_MAIN(ALog::Severity::Fatal)

#define ALOGV_N(N)                    ALOG_MODULE_N(N, ALog::Severity::Verbose)
#define ALOGD_N(N)                    ALOG_MODULE_N(N, ALog::Severity::Debug)
#define ALOGI_N(N)                    ALOG_MODULE_N(N, ALog::Severity::Info)
#define ALOGW_N(N)                    ALOG_MODULE_N(N, ALog::Severity::Warning)
#define ALOGE_N(N)                    ALOG_MODULE_N(N, ALog::Severity::Error)
#define ALOGF_N(N)                    ALOG_MODULE_N(N, ALog::Severity::Fatal)

#define ALOGMV_N(N)                   ALOG_MAIN_N(N, ALog::Severity::Verbose)
#define ALOGMD_N(N)                   ALOG_MAIN_N(N, ALog::Severity::Debug)
#define ALOGMI_N(N)                   ALOG_MAIN_N(N, ALog::Severity::Info)
#define ALOGMW_N(N)                   ALOG_MAIN_N(N, ALog::Severity::Warning)
#define ALOGME_N(N)                   ALOG_MAIN_N(N, ALog::Severity::Error)
#define ALOGMF_N(N)                   ALOG_MAIN_N(N, ALog::Severity::Fatal)

#define ALOG_TRACE                    ALOGD

// Conditional
#define ALOGV_IF(Cond)                ALOG_MODULE_IF(Cond, ALog::Severity::Verbose)
#define ALOGD_IF(Cond)                ALOG_MODULE_IF(Cond, ALog::Severity::Debug)
#define ALOGI_IF(Cond)                ALOG_MODULE_IF(Cond, ALog::Severity::Info)
#define ALOGW_IF(Cond)                ALOG_MODULE_IF(Cond, ALog::Severity::Warning)
#define ALOGE_IF(Cond)                ALOG_MODULE_IF(Cond, ALog::Severity::Error)
#define ALOGF_IF(Cond)                ALOG_MODULE_IF(Cond, ALog::Severity::Fatal)

#define ALOGMV_IF(Cond)               ALOG_MAIN_IF(Cond, ALog::Severity::Verbose)
#define ALOGMD_IF(Cond)               ALOG_MAIN_IF(Cond, ALog::Severity::Debug)
#define ALOGMI_IF(Cond)               ALOG_MAIN_IF(Cond, ALog::Severity::Info)
#define ALOGMW_IF(Cond)               ALOG_MAIN_IF(Cond, ALog::Severity::Warning)
#define ALOGME_IF(Cond)               ALOG_MAIN_IF(Cond, ALog::Severity::Error)
#define ALOGMF_IF(Cond)               ALOG_MAIN_IF(Cond, ALog::Severity::Fatal)

#define ALOGV_IF_N(N, Cond)           ALOG_MODULE_IF_N(N, Cond, ALog::Severity::Verbose)
#define ALOGD_IF_N(N, Cond)           ALOG_MODULE_IF_N(N, Cond, ALog::Severity::Debug)
#define ALOGI_IF_N(N, Cond)           ALOG_MODULE_IF_N(N, Cond, ALog::Severity::Info)
#define ALOGW_IF_N(N, Cond)           ALOG_MODULE_IF_N(N, Cond, ALog::Severity::Warning)
#define ALOGE_IF_N(N, Cond)           ALOG_MODULE_IF_N(N, Cond, ALog::Severity::Error)
#define ALOGF_IF_N(N, Cond)           ALOG_MODULE_IF_N(N, Cond, ALog::Severity::Fatal)

#define ALOGMV_IF_N(N, Cond)          ALOG_MAIN_IF_N(N, Cond, ALog::Severity::Verbose)
#define ALOGMD_IF_N(N, Cond)          ALOG_MAIN_IF_N(N, Cond, ALog::Severity::Debug)
#define ALOGMI_IF_N(N, Cond)          ALOG_MAIN_IF_N(N, Cond, ALog::Severity::Info)
#define ALOGMW_IF_N(N, Cond)          ALOG_MAIN_IF_N(N, Cond, ALog::Severity::Warning)
#define ALOGME_IF_N(N, Cond)          ALOG_MAIN_IF_N(N, Cond, ALog::Severity::Error)
#define ALOGMF_IF_N(N, Cond)          ALOG_MAIN_IF_N(N, Cond, ALog::Severity::Fatal)


// --- Short ---
#ifndef ALOG_NO_SHORT_MACROS
// Special
#define FLUSH                      ALOG_FL_FLUSH
#define THROW                      ALOG_FL_THROW
#define ABORT                      ALOG_FL_ABORT
#define NO_AUTO_QUOTES             ALOG_FL_NO_AUTO_QUOTES
#define SKIP_AUTO_QUOTES           ALOG_FL_SKIP_AUTO_QUOTES
#define PREFER_QUOTES              ALOG_FL_PREFER_QUOTES

#define BUFFER(ptr, sz)            ALOG_BUFFER(ptr, sz)

#define LOG_ASSERT(cond)           ALOG_ASSERT(cond)
#define LOG_ASSERT_D(cond)         ALOG_ASSERT_D(cond)
#define LOG_ASSERT_THROW(cond)     ALOG_ASSERT_THROW(cond)

// Main
#define LOGV                       ALOGV
#define LOGD                       ALOGD
#define LOGI                       ALOGI
#define LOGW                       ALOGW
#define LOGE                       ALOGE
#define LOGF                       ALOGF

#define LOGMV                      ALOGMV
#define LOGMD                      ALOGMD
#define LOGMI                      ALOGMI
#define LOGMW                      ALOGMW
#define LOGME                      ALOGME
#define LOGMF                      ALOGMF

#define LOGV_N(N)                  ALOGV_N(N)
#define LOGD_N(N)                  ALOGD_N(N)
#define LOGI_N(N)                  ALOGI_N(N)
#define LOGW_N(N)                  ALOGW_N(N)
#define LOGE_N(N)                  ALOGE_N(N)
#define LOGF_N(N)                  ALOGF_N(N)

#define LOGMV_N(N)                 ALOGMV_N(N)
#define LOGMD_N(N)                 ALOGMD_N(N)
#define LOGMI_N(N)                 ALOGMI_N(N)
#define LOGMW_N(N)                 ALOGMW_N(N)
#define LOGME_N(N)                 ALOGME_N(N)
#define LOGMF_N(N)                 ALOGMF_N(N)

#define TRACE                      ALOG_TRACE

// Short conditional
#define LOGV_IF(Cond)              ALOGV_IF(Cond)
#define LOGD_IF(Cond)              ALOGD_IF(Cond)
#define LOGI_IF(Cond)              ALOGI_IF(Cond)
#define LOGW_IF(Cond)              ALOGW_IF(Cond)
#define LOGE_IF(Cond)              ALOGE_IF(Cond)
#define LOGF_IF(Cond)              ALOGF_IF(Cond)

#define LOGMV_IF(Cond)             ALOGMV_IF(Cond)
#define LOGMD_IF(Cond)             ALOGMD_IF(Cond)
#define LOGMI_IF(Cond)             ALOGMI_IF(Cond)
#define LOGMW_IF(Cond)             ALOGMW_IF(Cond)
#define LOGME_IF(Cond)             ALOGME_IF(Cond)
#define LOGMF_IF(Cond)             ALOGMF_IF(Cond)

#define LOGV_IF_N(N, Cond)         ALOGV_IF_N(N, Cond)
#define LOGD_IF_N(N, Cond)         ALOGD_IF_N(N, Cond)
#define LOGI_IF_N(N, Cond)         ALOGI_IF_N(N, Cond)
#define LOGW_IF_N(N, Cond)         ALOGW_IF_N(N, Cond)
#define LOGE_IF_N(N, Cond)         ALOGE_IF_N(N, Cond)
#define LOGF_IF_N(N, Cond)         ALOGF_IF_N(N, Cond)

#define LOGMV_IF_N(N, Cond)        ALOGMV_IF_N(N, Cond)
#define LOGMD_IF_N(N, Cond)        ALOGMD_IF_N(N, Cond)
#define LOGMI_IF_N(N, Cond)        ALOGMI_IF_N(N, Cond)
#define LOGMW_IF_N(N, Cond)        ALOGMW_IF_N(N, Cond)
#define LOGME_IF_N(N, Cond)        ALOGME_IF_N(N, Cond)
#define LOGMF_IF_N(N, Cond)        ALOGMF_IF_N(N, Cond)
#endif // #ifndef ALOG_NO_SHORT_MACROS
