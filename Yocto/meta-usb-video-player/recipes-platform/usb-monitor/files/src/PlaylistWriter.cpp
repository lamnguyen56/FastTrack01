/** @file PlaylistWriter.cpp
 *  @brief Atomic publication of complete playlist snapshots.
 */
#include "PlaylistWriter.h"
#include <filesystem>
#include <fstream>
#include <unistd.h>
#include "Logger.h"

namespace usb {

bool PlaylistWriter::write(const std::vector<std::string>& files) const {
    // Rename on the same filesystem prevents a reader seeing a truncated snapshot.
    const std::string temporary = path_ + ".tmp." + std::to_string(getpid());
    std::ofstream out(temporary, std::ios::trunc);
    for (const std::string& file : files) {
        out << file << '\n';
    }
    out.close();

    std::error_code error;
    if (out.fail() == true) {
        LOG_E("Cannot write USB playlist snapshot: ", temporary);
        std::filesystem::remove(temporary, error);
        return false;
    }

    std::filesystem::rename(temporary, path_, error);
    if (error.value() != 0) {
        LOG_E("Cannot publish USB playlist: ", error.message());
        std::filesystem::remove(temporary, error);
        return false;
    }
    return true;
}

}
