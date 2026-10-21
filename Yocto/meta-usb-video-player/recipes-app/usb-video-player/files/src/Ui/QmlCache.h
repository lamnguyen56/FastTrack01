#pragma once
#include <QString>

/** @brief Isolate Qt disk caches by source content rather than reproducible RCC timestamps. */
class QmlCache {
public:
    /**
     * @brief Hash QML/JS sources and select their cache before constructing a QQmlEngine.
     * @param sourceRoot Resource subtree (or a filesystem directory for tests).
     * @param cacheRoot Optional cache parent for tests; defaults to the app cache location.
     * @return Selected cache directory, or empty when caching was disabled on failure.
     */
    static QString configure(const QString& sourceRoot = QStringLiteral(":/ui"),
                             const QString& cacheRoot = QString());
};
