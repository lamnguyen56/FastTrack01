/**
 * @file PlaylistWriter.cpp
 * @brief Playlist-file IPC publication.
 */

#include "PlaylistWriter.h"
#include <fstream>
#include <iostream>

namespace usb {

bool PlaylistWriter::write(const std::vector<std::string>& files) const {
    // Preserve inode compatibility with the existing QFileSystemWatcher client.
    std::ofstream out(path_, std::ios::trunc);
    for (const auto& file : files) {
        out << file << '\n';
    }
    out.flush();
    if (out.fail() == true) {
        std::cerr << "[usb] Cannot write playlist " << path_ << '\n';
    }

    return out.fail() == false;
}

}
