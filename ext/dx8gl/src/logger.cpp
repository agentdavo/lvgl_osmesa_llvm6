#include "logger.h"

namespace dx8gl {

// Logger implementation is header-only for performance
// This file exists to ensure the logger is linked into the library

void init_logging() {
    // Initialize logging subsystem
    Logger::instance().set_level(LogLevel::DEBUG);
    Logger::instance().enable_timestamps(false);  // Disable timestamps per user request
    Logger::instance().enable_thread_ids(false);  // Disable thread IDs to clean up output
    
    DX8GL_INFO("dx8gl logging system initialized");
}

} // namespace dx8gl