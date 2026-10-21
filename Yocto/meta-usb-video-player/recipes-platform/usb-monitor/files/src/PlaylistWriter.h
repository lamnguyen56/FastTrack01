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
 * @brief Publish a complete playlist-file IPC snapshot using atomic rename.
 * @note Readers must rearm file watches after replacement; the Qt client also polls.
 */
class PlaylistWriter {

public:
    /**
     * @brief Configure the playlist output file.
     * @param path Writable IPC file watched by the current Qt client.
     */
    explicit PlaylistWriter(std::string path) : path_(std::move(path)) {}
    /**
     * @brief Write newline-separated media paths to a temporary file and publish atomically.
     * @param files Ordered media paths to publish.
     * @return True when the stream reports a successful write and flush.
     */
    bool write(const std::vector<std::string>& files) const;

    /**
     * @brief Publish an empty playlist.
     * @return True on successful publication of an empty snapshot.
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
