/**
 * @file Logger.h
 * @brief Header-only, thread-safe stdout logger for platform services.
 * @note Configure LOG_LEVEL before startup or call logging::setLevel().
 * Never call this logger from a signal handler. Journald priority is not
 * inferred from these text labels; all messages use the service stdout stream.
 */
#pragma once

#include <atomic>
#include <chrono>
#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <ctime>
#include <mutex>
#include <sstream>
#include <utility>

namespace logging {

/** @brief Minimum enabled severity; Off disables all output. */
enum class Level { Debug = 0, Info, Warn, Error, Off };

/**
 * @brief Parse the process environment once on first logger use.
 * @return INFO if absent or unrecognized; otherwise the configured threshold.
 */
inline Level environmentLevel() {
    const char* value = std::getenv("LOG_LEVEL");
    if (value == nullptr) { return Level::Info; }
    if (std::strcmp(value, "DEBUG") == 0) { return Level::Debug; }
    if (std::strcmp(value, "INFO") == 0) { return Level::Info; }
    if (std::strcmp(value, "WARN") == 0) { return Level::Warn; }
    if (std::strcmp(value, "ERROR") == 0) { return Level::Error; }
    if (std::strcmp(value, "OFF") == 0) { return Level::Off; }
    return Level::Info;
}

/** @brief Shared per-process threshold, initialized using LOG_LEVEL. */
inline std::atomic<Level>& threshold() {
    static std::atomic<Level> value{environmentLevel()};
    return value;
}

/** @brief Change the threshold for subsequent messages in this process. */
inline void setLevel(Level level) { threshold().store(level); }

/** @brief Return the active severity threshold. */
inline Level getLevel() { return threshold().load(); }

/** @brief Check severity before evaluating message arguments. */
inline bool enabled(Level level) {
    const Level minimum = getLevel();
    return minimum != Level::Off && level >= minimum && level < Level::Off;
}

/** @brief Convert a severity to its printable label. */
inline const char* label(Level level) {
    switch (level) {
    case Level::Debug: return "DEBUG";
    case Level::Info: return "INFO";
    case Level::Warn: return "WARN";
    case Level::Error: return "ERROR";
    default: return "OFF";
    }
}

/** @brief Serialize complete lines from all threads using this logger. */
inline std::mutex& outputMutex() {
    static std::mutex mutex;
    return mutex;
}

/**
 * @brief Stream arguments into one message, write to stdout and flush.
 * @param level Message severity.
 * @param file Source file supplied by the macro.
 * @param line Source line supplied by the macro.
 * @param args Streamable values, separated by the caller's literal text.
 * @note API uses stream-style arguments, not printf format substitution.
 */
template<class... Args>
inline void write(Level level, const char* file, int line, Args&&... args) {
    std::ostringstream message;
    (message << ... << std::forward<Args>(args));
    const char* filename = std::strrchr(file, '/');
    filename = filename == nullptr ? file : filename + 1;

    // Build output under the mutex so timestamps and lines cannot interleave.
    const std::lock_guard<std::mutex> lock(outputMutex());
    const std::time_t now = std::chrono::system_clock::to_time_t(std::chrono::system_clock::now());
    std::tm local{};
    localtime_r(&now, &local);
    char timestamp[32]{};
    std::strftime(timestamp, sizeof(timestamp), "%Y-%m-%d %H:%M:%S", &local);
    std::string text = message.str();
    // Keep each call on one physical line for journal and console readers.
    for (char& character : text) {
        if (character == '\n' || character == '\r') { character = ' '; }
    }
    std::fprintf(stdout, "%s [%s] %s:%d %s\n", timestamp, label(level), filename, line, text.c_str());
    std::fflush(stdout);
}

} // namespace logging

/** @brief Log only enabled messages; suppressed arguments are not evaluated. */
#define LOG_AT(level, ...) \
    do { \
        if (::logging::enabled(level) == true) { \
            ::logging::write(level, __FILE__, __LINE__, __VA_ARGS__); \
        } \
    } while (false)

/** @brief Debug details for hotplug and diagnosis. */
#define LOG_D(...) LOG_AT(::logging::Level::Debug, __VA_ARGS__)
/** @brief Normal lifecycle/progress messages. */
#define LOG_I(...) LOG_AT(::logging::Level::Info, __VA_ARGS__)
/** @brief Recoverable unexpected conditions. */
#define LOG_W(...) LOG_AT(::logging::Level::Warn, __VA_ARGS__)
/** @brief Failed operations requiring attention. */
#define LOG_E(...) LOG_AT(::logging::Level::Error, __VA_ARGS__)
