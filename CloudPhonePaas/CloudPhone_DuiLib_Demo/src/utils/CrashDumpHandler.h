#pragma once

/**
 * @brief Windows MiniDump crash handler.
 *
 * Registers an unhandled exception filter that generates .dmp files
 * in <exe_dir>/dumps/ when the application crashes.
 */
class CrashDumpHandler {
public:
    static void initialize();
};
