/**
 * @file PlaylistWriter.cpp
 * @brief Playlist-file IPC publication.
 */

#include "PlaylistWriter.h"
#include <fstream>
#include "Logger.h"

namespace usb {

bool PlaylistWriter::write(const std::vector<std::string>& files) const {
    // Preserve inode compatibility with the existing QFileSystemWatcher client.
    std::ofstream out(path_, std::ios::trunc);
    for (const auto& file : files) {
        out << file << '\n';
    }
    out.flush();
    if (out.fail() == true) {
        LOG_E("[usb] Cannot write playlist ", path_);
    }

    return out.fail() == false;
}

}
