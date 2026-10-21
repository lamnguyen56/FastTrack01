#include "UsbPlaylistModel.h"
#include <QFileInfo>

UsbPlaylistModel::UsbPlaylistModel(QObject* parent) : QAbstractListModel(parent) {
}

int UsbPlaylistModel::rowCount(const QModelIndex& parent) const {
    return parent.isValid() == true ? 0 : files_.size();
}

QVariant UsbPlaylistModel::data(const QModelIndex& index, int role) const {
    if (index.isValid() == false || index.row() < 0 || index.row() >= files_.size()) {
        return QVariant();
    }

    if (role == TitleRole) {
        return QFileInfo(files_.at(index.row())).fileName();
    }
    if (role == FilePathRole) {
        return files_.at(index.row());
    }
    return QVariant();
}

QHash<int, QByteArray> UsbPlaylistModel::roleNames() const {
    return {{TitleRole, "title"}, {FilePathRole, "filePath"}};
}

int UsbPlaylistModel::count() const {
    return files_.size();
}

QString UsbPlaylistModel::pathAt(int row) const {
    if (row < 0 || row >= files_.size()) {
        return QString();
    }
    return files_.at(row);
}

int UsbPlaylistModel::indexOf(const QString& path) const {
    return files_.indexOf(path);
}

void UsbPlaylistModel::replaceFiles(const QStringList& files) {
    if (files_ == files) {
        return;
    }

    beginResetModel();
    files_ = files;
    endResetModel();
    emit playlistChanged();
}
