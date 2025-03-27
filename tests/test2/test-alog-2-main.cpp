/* License:  MIT
 * Source:   https://github.com/ihor-drachuk/alog
 * Contact:  ihor-drachuk-libs@pm.me  */

#include <alog/logger.h>


int main()
{
    SIMPLE_SETUP_ALOG;
    LOGMD << "Test";
    LOGMI << "Info";
    LOGMW << "Warning";
    LOGME << "Error";
    LOGMF << "Fatal";
    LOGM_ASSERT(true);
    LOGM_ASSERT(true) << "1";
    LOGM_ASSERT_D(true);
    LOGM_ASSERT_D(true) << "2";
    return 0;
}
