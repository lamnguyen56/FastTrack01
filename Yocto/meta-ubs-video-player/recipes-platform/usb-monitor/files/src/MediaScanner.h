/**
 * @file MediaScanner.h
 * @brief Asynchronous cancellable media discovery.
 */

#pragma once

#include <atomic>
#include <future>
#include <memory>
#include <optional>
#include <string>
#include <vector>

namespace usb {

/**
 * @brief Completed scan snapshot; invalid results must never become a playlist.
 */
struct ScanResult {
    /**
     * @brief True only when traversal completed without cancellation or I/O error.
     */
    bool valid = false;

    /**
     * @brief Sorted media paths; an empty valid list represents empty storage.
     */
    std::vector<std::string> files;
};

/**
 * @brief Owns one asynchronous recursive media scan and its cancellation token.
 * @note start(), cancel() and poll() are event-loop-thread operations; only scan() runs in the worker.
 */
class MediaScanner {

public:
    /**
     * @brief Request cancellation and join the worker.
     * @warning Blocking kernel I/O may delay shutdown despite cancellation.
     */
    ~MediaScanner();

    /**
     * @brief Report whether a job exists, including a completed job not yet collected.
     * @return True until poll() consumes the result.
     */
    bool busy() const {
        return job_.valid();
    }

    /**
     * @brief Start one worker without blocking the event loop.
     * @param path Mounted directory to traverse.
     * @return False if another job exists or worker creation fails.
     */
    bool start(const std::string& path);

    /**
     * @brief Request cooperative cancellation; this does not wait for the worker.
     */
    void cancel();

    /**
     * @brief Collect a ready result without waiting.
     * @return No value while idle/pending; otherwise a valid or failed/cancelled snapshot.
     */
    std::optional<ScanResult> poll();

    /**
     * @brief Traverse media candidates without following symlinks.
     * @param path Root directory of the selected mount.
     * @param cancel Shared, non-null cancellation flag checked during traversal.
     * @return Sorted paths with valid=true on success, or an invalid snapshot on error/cancel.
     * @note Extension filtering does not verify that a codec can decode the file.
     */
    static ScanResult scan(const std::string& path, const std::shared_ptr<std::atomic<bool>>& cancel);

private:
    /**
     * @brief Worker result handle; consumed only by poll() or joined on destruction.
     */
    std::future<ScanResult> job_;

    /**
     * @brief Cancellation token shared with the currently owned worker.
     */
    std::shared_ptr<std::atomic<bool>> cancel_;
};

}
