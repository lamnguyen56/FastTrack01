#pragma once

#include <QAbstractListModel>
#include <QStringList>

/** @brief Ordered USB media snapshot exposed to the QML playlist. */
class UsbPlaylistModel : public QAbstractListModel {
    Q_OBJECT
    Q_PROPERTY(int count READ count NOTIFY playlistChanged)
    Q_PROPERTY(QStringList titles READ titles NOTIFY playlistChanged)

public:
    /** @brief Roles used by the QML delegate. */
    enum Role { TitleRole = Qt::UserRole + 1, FilePathRole };

    /** @brief Construct an initially empty model. */
    explicit UsbPlaylistModel(QObject* parent = nullptr);

    /** @brief Return the number of top-level media rows. */
    int rowCount(const QModelIndex& parent = QModelIndex()) const override;

    /** @brief Return the requested media role, or an invalid value for a bad index. */
    QVariant data(const QModelIndex& index, int role) const override;

    /** @brief Map role identifiers to QML property names. */
    QHash<int, QByteArray> roleNames() const override;

    /** @brief Return the number of files in the published snapshot. */
    int count() const;

    /** @brief Cached display names, published with the snapshot for view sizing. */
    QStringList titles() const;

    /** @brief Return a file path, or an empty string for an invalid row. */
    QString pathAt(int row) const;

    /** @brief Find a previously selected path after a snapshot update. */
    int indexOf(const QString& path) const;

    /** @brief Replace the snapshot only when its contents actually change. */
    void replaceFiles(const QStringList& files);

signals:
    /** @brief Emitted after a different complete snapshot has been installed. */
    void playlistChanged();

private:
    QStringList files_; ///< Canonical local paths, in publisher order.
    QStringList titles_; ///< File names without directory prefixes, in the same order.
};
