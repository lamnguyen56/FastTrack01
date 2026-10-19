/**
 * @file PlaylistWriter.h
 * @brief Playlist-file IPC publication.
 */

#pragma once

#include <string>
#include <vector>
#include <utility>

namespace usb {

/**
 * @brief Publish playlist-file IPC while preserving the file inode.
 * @note Writes are not atomic snapshots; transport migration is outside this module.
 */
class PlaylistWriter {

public:
    /**
     * @brief Configure the playlist output file.
     * @param path Writable IPC file watched by the current Qt client.
     */
    explicit PlaylistWriter(std::string path) : path_(std::move(path)) {}
    /**
     * @brief Truncate and write newline-separated media paths.
     * @param files Ordered media paths to publish.
     * @return True when the stream reports a successful write and flush.
     */
    bool write(const std::vector<std::string>& files) const;

    /**
     * @brief Publish an empty playlist.
     * @return True on successful truncation and flush.
     */
    bool clear() const {
        return write({});
    }

private:
    /**
     * @brief Destination of the existing playlist-file IPC.
     */
    std::string path_;
};

}
