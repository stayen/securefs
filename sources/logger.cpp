#include "logger.h"
#include "myutils.h"

#include <format.h>

#include <thread>

namespace securefs
{
void Logger::log(LoggingLevel level,
                 const std::string& msg,
                 const char* func,
                 const char* file,
                 int line) noexcept
{
    try
    {
        auto full_msg = fmt::format("[{}] [{}] [{}:{}] [{}]      {}\n",
                                    stringify(level),
                                    format_current_time(),
                                    file,
                                    line,
                                    func,
                                    msg);
        append(full_msg.data(), full_msg.size());
    }
    catch (...)
    {
        // Cannot handle errors in logging
    }
}
}
