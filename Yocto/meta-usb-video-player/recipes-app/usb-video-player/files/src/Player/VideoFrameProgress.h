#pragma once

#include <cstdint>

/** @brief Distinguish new URI frame delivery from retained counters and transport state. */
class VideoFrameProgress {
public:
    /** @brief Start a URI at a known sink counter without claiming a displayed frame. */
    void reset(std::uint64_t baseline = 0) {
        previous_ = baseline;
        started_ = false;
    }

    /** @brief Return true only for new rendered buffers; tolerate a sink counter reset. */
    bool observe(std::uint64_t rendered) {
        if (rendered < previous_) {
            previous_ = 0;
        }
        const bool advanced = rendered > previous_;
        previous_ = rendered;
        started_ = started_ || advanced;
        return advanced;
    }

    /** @brief Keep first-frame state across pause/EOS until an explicit URI reset. */
    bool started() const { return started_; }

private:
    std::uint64_t previous_ = 0; ///< Last rendered counter for the current URI.
    bool started_ = false; ///< New buffer delivery has been observed.
};
