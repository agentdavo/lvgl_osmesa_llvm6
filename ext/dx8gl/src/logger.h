#ifndef DX8GL_LOGGER_H
#define DX8GL_LOGGER_H

#include <cstdio>
#include <cstdarg>
#include <cstring>
#include <mutex>
#include <thread>
#include <chrono>
#include <string>
#include <atomic>

#ifdef __EMSCRIPTEN__
#include <emscripten.h>
#endif

// Macro to suppress unused parameter warnings
#define UNUSED(x) ((void)(x))

namespace dx8gl {

enum class LogLevel {
    TRACE = 0,
    DEBUG = 1,
    INFO = 2,
    WARNING = 3,
    ERROR = 4,
    FATAL = 5
};

class Logger {
public:
    static Logger& instance() {
        static Logger logger;
        return logger;
    }

    void set_level(LogLevel level) { min_level_ = level; }
    void set_file(FILE* file) { 
        std::lock_guard<std::mutex> lock(mutex_);
        log_file_ = file; 
    }
    void enable_timestamps(bool enable) { timestamps_enabled_ = enable; }
    void enable_thread_ids(bool enable) { thread_ids_enabled_ = enable; }

    void log(LogLevel level, const char* file, int line, const char* func, const char* fmt, ...) {
        if (level < min_level_) return;

        va_list args;
        va_start(args, fmt);
        log_impl(level, file, line, func, fmt, args);
        va_end(args);
    }

    void trace(const char* file, int line, const char* func, const char* fmt, ...) {
        if (LogLevel::TRACE < min_level_) return;
        va_list args;
        va_start(args, fmt);
        log_impl(LogLevel::TRACE, file, line, func, fmt, args);
        va_end(args);
    }

    void debug(const char* file, int line, const char* func, const char* fmt, ...) {
        if (LogLevel::DEBUG < min_level_) return;
        va_list args;
        va_start(args, fmt);
        log_impl(LogLevel::DEBUG, file, line, func, fmt, args);
        va_end(args);
    }

    void info(const char* file, int line, const char* func, const char* fmt, ...) {
        if (LogLevel::INFO < min_level_) return;
        va_list args;
        va_start(args, fmt);
        log_impl(LogLevel::INFO, file, line, func, fmt, args);
        va_end(args);
    }

    void warning(const char* file, int line, const char* func, const char* fmt, ...) {
        if (LogLevel::WARNING < min_level_) return;
        va_list args;
        va_start(args, fmt);
        log_impl(LogLevel::WARNING, file, line, func, fmt, args);
        va_end(args);
    }

    void error(const char* file, int line, const char* func, const char* fmt, ...) {
        if (LogLevel::ERROR < min_level_) return;
        va_list args;
        va_start(args, fmt);
        log_impl(LogLevel::ERROR, file, line, func, fmt, args);
        va_end(args);
    }

    void fatal(const char* file, int line, const char* func, const char* fmt, ...) {
        va_list args;
        va_start(args, fmt);
        log_impl(LogLevel::FATAL, file, line, func, fmt, args);
        va_end(args);
    }

private:
    Logger() : log_file_(stderr), min_level_(LogLevel::INFO), 
               timestamps_enabled_(false), thread_ids_enabled_(false) {}

    void log_impl(LogLevel level, const char* file, int line, const char* func, 
                  const char* fmt, va_list args) {
        std::lock_guard<std::mutex> lock(mutex_);
        
#ifdef __EMSCRIPTEN__
        // For Emscripten, output to JavaScript console
        char buffer[2048];
        char msg_buffer[1024];
        
        // Format the actual message
        vsnprintf(msg_buffer, sizeof(msg_buffer), fmt, args);
        
        // Build complete log message
        const char* basename = strrchr(file, '/');
        if (!basename) basename = strrchr(file, '\\');
        if (!basename) basename = file;
        else basename++;
        
        snprintf(buffer, sizeof(buffer), "[%s] %s:%d in %s(): %s", 
                 level_to_string(level), basename, line, func, msg_buffer);
        
        // Output to JavaScript console based on log level
        switch (level) {
            case LogLevel::ERROR:
            case LogLevel::FATAL:
                EM_ASM({ console.error(UTF8ToString($0)); }, buffer);
                break;
            case LogLevel::WARNING:
                EM_ASM({ console.warn(UTF8ToString($0)); }, buffer);
                break;
            case LogLevel::INFO:
                EM_ASM({ console.info(UTF8ToString($0)); }, buffer);
                break;
            case LogLevel::DEBUG:
            case LogLevel::TRACE:
                EM_ASM({ console.log(UTF8ToString($0)); }, buffer);
                break;
        }
#else
        if (!log_file_) return;

        // Print timestamp if enabled
        if (timestamps_enabled_) {
            auto now = std::chrono::system_clock::now();
            auto time_t = std::chrono::system_clock::to_time_t(now);
            auto ms = std::chrono::duration_cast<std::chrono::milliseconds>(
                now.time_since_epoch()) % 1000;
            
            char time_buf[32];
            strftime(time_buf, sizeof(time_buf), "%Y-%m-%d %H:%M:%S", localtime(&time_t));
            fprintf(log_file_, "[%s.%03d] ", time_buf, static_cast<int>(ms.count()));
        }

        // Print thread ID if enabled
        if (thread_ids_enabled_) {
            fprintf(log_file_, "[%08x] ", get_thread_id());
        }

        // Print log level
        fprintf(log_file_, "[%s] ", level_to_string(level));

        // Print location
        const char* basename = strrchr(file, '/');
        if (!basename) basename = strrchr(file, '\\');
        if (!basename) basename = file;
        else basename++;
        
        fprintf(log_file_, "%s:%d in %s(): ", basename, line, func);

        // Print actual message
        vfprintf(log_file_, fmt, args);
        fprintf(log_file_, "\n");
        fflush(log_file_);
#endif
    }

    const char* level_to_string(LogLevel level) {
        switch (level) {
            case LogLevel::TRACE:   return "TRACE";
            case LogLevel::DEBUG:   return "DEBUG";
            case LogLevel::INFO:    return "INFO ";
            case LogLevel::WARNING: return "WARN ";
            case LogLevel::ERROR:   return "ERROR";
            case LogLevel::FATAL:   return "FATAL";
            default:                return "?????";
        }
    }

    uint32_t get_thread_id() {
        static std::atomic<uint32_t> next_id{1};
        static thread_local uint32_t thread_id = next_id.fetch_add(1);
        return thread_id;
    }

    std::mutex mutex_;
    FILE* log_file_;
    std::atomic<LogLevel> min_level_;
    std::atomic<bool> timestamps_enabled_;
    std::atomic<bool> thread_ids_enabled_;
};

// Convenience macros
#define DX8GL_LOG(level, ...) \
    ::dx8gl::Logger::instance().log(level, __FILE__, __LINE__, __func__, __VA_ARGS__)

#define DX8GL_TRACE(...) \
    ::dx8gl::Logger::instance().trace(__FILE__, __LINE__, __func__, __VA_ARGS__)

#define DX8GL_DEBUG(...) \
    ::dx8gl::Logger::instance().debug(__FILE__, __LINE__, __func__, __VA_ARGS__)

#define DX8GL_INFO(...) \
    ::dx8gl::Logger::instance().info(__FILE__, __LINE__, __func__, __VA_ARGS__)

#define DX8GL_WARNING(...) \
    ::dx8gl::Logger::instance().warning(__FILE__, __LINE__, __func__, __VA_ARGS__)

#define DX8GL_WARN(...) \
    ::dx8gl::Logger::instance().warning(__FILE__, __LINE__, __func__, __VA_ARGS__)

#define DX8GL_ERROR(...) \
    ::dx8gl::Logger::instance().error(__FILE__, __LINE__, __func__, __VA_ARGS__)

#define DX8GL_FATAL(...) \
    ::dx8gl::Logger::instance().fatal(__FILE__, __LINE__, __func__, __VA_ARGS__)

} // namespace dx8gl

#endif // DX8GL_LOGGER_H