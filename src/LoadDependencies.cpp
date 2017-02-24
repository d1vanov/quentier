#include "LoadDependencies.h"
#include <quentier/logging/QuentierLogger.h>
#include <quentier/utility/VersionInfo.h>
#include <QCoreApplication>
#include <QFileInfo>
#include <QPluginLoader>
#include <QStringList>
#include <QDirIterator>
#include <QSqlDatabase>

namespace quentier {

void loadDependencies()
{
    QNDEBUG(QStringLiteral("loadDependencies"));

#ifdef Q_OS_WIN
    QStringList paths = QCoreApplication::libraryPaths();
    paths.append(QStringLiteral("."));
    paths.append(QStringLiteral("imageformats"));
    paths.append(QStringLiteral("platforms"));
    paths.append(QStringLiteral("sqldrivers"));
    QCoreApplication::setLibraryPaths(paths);

    // Need to load the SQL drivers manually, for some reason Qt doesn't wish to load them on its own
    QDirIterator sqlDriversIter(QStringLiteral("sqldrivers"));
    while(sqlDriversIter.hasNext())
    {
        QString fileName = sqlDriversIter.next();
        if ( (fileName == QStringLiteral("sqldrivers/.")) ||
             (fileName == QStringLiteral("sqldrivers/..")) )
        {
            continue;
        }

        QPluginLoader pluginLoader(fileName);
        if (!pluginLoader.load()) {
            QNDEBUG(QStringLiteral("Failed to load plugin ") << fileName);
        }
        else {
            QNDEBUG(QStringLiteral("Loaded plugin ") << fileName);
        }
    }

    QStringList drivers = QSqlDatabase::drivers();
    for(auto it = drivers.constBegin(), end = drivers.constEnd(); it != end; ++it) {
        QNDEBUG(QStringLiteral("Available SQL driver: ") << *it);
    }

#ifdef LIB_QUENTIER_USE_QT_WEB_ENGINE
    QString qtWebEngineProcessPath = QDir::currentPath() + QStringLiteral("/QtWebEngineProcess.exe");
    QFileInfo qtWebEngineProcessFileInfo(qtWebEngineProcessPath);
    if (!qtWebEngineProcessFileInfo.exists()) {
        qtWebEngineProcessPath = QDir::currentPath() + QStringLiteral("/QtWebEngineProcessd.exe");
    }

    qputenv("QTWEBENGINEPROCESS_PATH", QByteArray(qWebEngineProcessPath.toLocal8Bit()));
    QNDEBUG(QStringLiteral("Set QTWEBENGINEPROCESS_PATH to ") << qWebEngineProcessPath);
#endif

#endif // Q_OS_WIN
}

} // namespace quentier
