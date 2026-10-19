// Test checks must remain enabled even when built with -DNDEBUG.
#ifdef NDEBUG
#undef NDEBUG
#endif

// Host-only test: no device mounts or root privileges required.
#include "../files/src/MediaScanner.h"
#include <filesystem>
#include <fstream>
#include <algorithm>
#include <iostream>
#include <thread>
#include <chrono>
namespace fs = std::filesystem;
#include <cassert>
#include <unistd.h>

int main() {
    const auto root = fs::temp_directory_path() / ("usb-scan-test-" + std::to_string(getpid()));
    fs::create_directories(root / "nested");
    std::ofstream(root / "b.MP4") << "sample";
    std::ofstream(root / "nested/a.mkv") << "sample";
    std::ofstream(root / "ignore.txt") << "text";
    fs::create_symlink(root / "b.MP4", root / "alias.mp4");
    fs::create_directory_symlink(root / "nested", root / "alias-dir");
    auto cancel = std::make_shared<std::atomic<bool>>(false);
    auto result = usb::MediaScanner::scan(root.string(), cancel);
    assert(result.valid && result.files.size() == 2);
    assert(std::is_sorted(result.files.begin(), result.files.end()));
    cancel->store(true);
    assert(!usb::MediaScanner::scan(root.string(), cancel).valid);
    cancel->store(false);
    fs::remove_all(root);
    assert(!usb::MediaScanner::scan(root.string(), cancel).valid);
    fs::create_directory(root);
    result = usb::MediaScanner::scan(root.string(), cancel);
    assert(result.valid && result.files.empty());
    usb::MediaScanner worker;
    assert(worker.start(root.string()));
    assert(!worker.start(root.string())); // No overlapping scan jobs.
    worker.cancel();
    std::optional<usb::ScanResult> completed;
    for (int i = 0; i < 1000 && !completed; ++i) {
        completed = worker.poll();
        std::this_thread::sleep_for(std::chrono::milliseconds(1));
    }
    assert(completed && !completed->valid && !worker.busy());
    fs::remove(root);
    std::cout << "PASS: media filtering/sort, no symlink traversal, cancellation, missing/empty storage\n";
}
