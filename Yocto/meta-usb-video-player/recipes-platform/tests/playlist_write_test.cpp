// Verify atomic publication and failure cleanup without touching the real IPC path.
#ifdef NDEBUG
#undef NDEBUG
#endif
#include <cassert>
#include <cstdlib>
#include <filesystem>
#include <fstream>
#include <iterator>
#include <string>
#include "../usb-monitor/files/src/PlaylistWriter.h"

static std::string contents(const std::filesystem::path& path) {
    std::ifstream input(path);
    return std::string(std::istreambuf_iterator<char>(input), std::istreambuf_iterator<char>());
}

int main() {
    char pattern[] = "/tmp/usb-playlist-writer-XXXXXX";
    const char* created = mkdtemp(pattern);
    assert(created != nullptr);
    const std::filesystem::path root(created);
    const std::filesystem::path output = root / "playlist.txt";
    usb::PlaylistWriter writer(output.string());
    assert(writer.write({"/media/usb0/a video.mp4", "/media/usb0/b.mkv"}) == true);
    assert(contents(output) == "/media/usb0/a video.mp4\n/media/usb0/b.mkv\n");

    // An existing open reader keeps its complete old snapshot after replacement.
    std::ifstream oldSnapshot(output);
    assert(writer.write({"/media/usb0/new.mp4"}) == true);
    const std::string previous{std::istreambuf_iterator<char>(oldSnapshot), std::istreambuf_iterator<char>()};
    assert(previous == "/media/usb0/a video.mp4\n/media/usb0/b.mkv\n");
    assert(contents(output) == "/media/usb0/new.mp4\n");
    assert(writer.clear() == true);
    assert(contents(output).empty() == true);

    const std::filesystem::path blocked = root / "directory";
    std::filesystem::create_directory(blocked);
    usb::PlaylistWriter failed(blocked.string());
    assert(failed.write({"ignored"}) == false);
    assert(std::filesystem::is_directory(blocked) == true);
    for (const std::filesystem::directory_entry& entry : std::filesystem::directory_iterator(root)) {
        assert(entry.path().filename().string().find(".tmp.") == std::string::npos);
    }
    std::filesystem::remove_all(root);
    return 0;
}
