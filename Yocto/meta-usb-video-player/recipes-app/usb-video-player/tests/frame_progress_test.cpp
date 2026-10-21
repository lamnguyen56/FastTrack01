#ifdef NDEBUG
#undef NDEBUG
#endif
#include <cassert>
#include "Player/VideoFrameProgress.h"

/** @brief A PLAYING transport or stale previous-URI count must not hide loading. */
int main() {
    VideoFrameProgress frames;
    assert(frames.started() == false);
    assert(frames.observe(0) == false);
    assert(frames.started() == false);
    assert(frames.observe(1) == true);
    assert(frames.started() == true);
    assert(frames.observe(1) == false);
    assert(frames.started() == true); // Pause and held EOS retain the first-frame state.

    frames.reset(42);
    assert(frames.started() == false);
    assert(frames.observe(42) == false); // Retained counters are not a new frame.
    assert(frames.observe(0) == false);  // READY/PAUSED may reset the sink counter.
    assert(frames.started() == false);
    assert(frames.observe(3) == true);
    assert(frames.started() == true);

    frames.reset(90);
    assert(frames.observe(2) == true); // Counter reset plus real frames in one poll.
    frames.reset();
    assert(frames.started() == false); // USB removal/source replacement clears state.
    assert(frames.observe(0) == false);
}
