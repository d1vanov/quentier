#include "FileSystemWatcher_p.h"
#include <qute_note/logging/QuteNoteLogger.h>
#include <QFileInfo>
#include <QTimerEvent>

namespace qute_note {

FileSystemWatcherPrivate::FileSystemWatcherPrivate(FileSystemWatcher & parent, const int removalTimeoutMSec) :
    QObject(&parent),
    m_parent(parent),
    m_watcher(),
    m_removalTimeoutMSec(removalTimeoutMSec),
    m_watchedFilesWithDirs(),
    m_watchedDirectories(),
    m_justRemovedFilePathsWithPostRemovalTimerIds(),
    m_justRemovedDirectoryPathsWithPostRemovalTimerIds()
{
    createConnections();
}

FileSystemWatcherPrivate::FileSystemWatcherPrivate(FileSystemWatcher & parent, const QStringList & paths, const int removalTimeoutMSec) :
    QObject(&parent),
    m_parent(parent),
    m_watcher(paths),
    m_removalTimeoutMSec(removalTimeoutMSec),
    m_watchedFilesWithDirs(),
    m_justRemovedFilePathsWithPostRemovalTimerIds(),
    m_justRemovedDirectoryPathsWithPostRemovalTimerIds()
{
    createConnections();
}

FileSystemWatcherPrivate::~FileSystemWatcherPrivate()
{}

void FileSystemWatcherPrivate::addPath(const QString & path)
{
    QFileInfo info(path);
    if (info.isFile()) {
        QString directoryPath = info.absolutePath();
        Q_UNUSED(m_watchedFilesWithDirs.insert(WatchedFilePathWithDirPaths::value_type(path, directoryPath)));
    }
    else if (info.isDir()) {
        Q_UNUSED(m_watchedDirectories.insert(path));
    }

    m_watcher.addPath(path);
}

void FileSystemWatcherPrivate::addPaths(const QStringList & paths)
{
    const int numPaths = paths.size();
    for(int i = 0; i < numPaths; ++i) {
        addPath(paths[i]);
    }
}

QStringList FileSystemWatcherPrivate::directories() const
{
    return m_watcher.directories();
}

QStringList FileSystemWatcherPrivate::files() const
{
    return m_watcher.files();
}

void FileSystemWatcherPrivate::removePath(const QString & path)
{
    auto fileIt = m_watchedFilesWithDirs.left.find(path);
    if (fileIt != m_watchedFilesWithDirs.left.end()) {
        Q_UNUSED(m_watchedFilesWithDirs.left.erase(fileIt));
    }
    else {
        auto dirIt = m_watchedDirectories.find(path);
        if (dirIt != m_watchedDirectories.end()) {
            Q_UNUSED(m_watchedDirectories.erase(dirIt));
        }
    }

    m_watcher.removePath(path);
}

void FileSystemWatcherPrivate::removePaths(const QStringList & paths)
{
    const int numPaths = paths.size();
    for(int i = 0; i < numPaths; ++i) {
        removePath(paths[i]);
    }
}

void FileSystemWatcherPrivate::onFileChanged(const QString & path)
{
    QNTRACE("FileSystemWatcherPrivate::onFileChanged: " << path);

    auto fileIt = m_watchedFilesWithDirs.left.find(path);
    if (Q_UNLIKELY(fileIt == m_watchedFilesWithDirs.left.end())) {
        QNWARNING("Received file changed event for file not listed as watched");
        return;
    }

    QFileInfo info(path);
    if (!info.isFile()) {
        processFileRemoval(path);
    }
    else {
        emit fileChanged(path);
        m_watcher.addPath(path);
    }
}

void FileSystemWatcherPrivate::onDirectoryChanged(const QString & path)
{
    QNTRACE("FileSystemWatcherPrivate::onDirectoryChanged: " << path);

    auto dirIt = m_watchedDirectories.find(path);
    if (dirIt != m_watchedDirectories.end())
    {
        QFileInfo info(path);
        if (!info.isDir()) {
            processDirectoryRemoval(path);
        }
        else {
            emit directoryChanged(path);
            m_watcher.addPath(path);
        }

        return;
    }

    auto implicitDirIt = m_watchedFilesWithDirs.right.find(path);
    if (implicitDirIt != m_watchedFilesWithDirs.right.end())
    {
        QFileInfo info(path);
        if (!info.isDir())
        {
            QNTRACE("Implicitly watched directory was removed");
            // TODO: need to either consider all files removed right now
            // or set up the removal timeout timer to track this directory's removal
            // and possible re-appear again
        }
        else
        {
            // TODO: check if some files being watched are no longer present in the directory;
            // for them need to set up removal timeout timers
            /*
            QDir dir = info.absoluteDir();
            QFileInfoList fileInfos = dir.entryInfoList(QDir::Files);
            const int numFiles = fileInfos.size();
            for(int i = 0; i < numFiles; ++i)
            {
                const QFileInfo & fileInfo = fileInfos[i];
                const QString & absoluteFilePath = fileInfo.absoluteFilePath();

            }
            */
        }

        return;
    }
}

void FileSystemWatcherPrivate::createConnections()
{
    QObject::connect(this, QNSIGNAL(FileSystemWatcherPrivate,fileChanged,const QString&),
                     &m_parent, QNSIGNAL(FileSystemWatcher,fileChanged,const QString&));
    QObject::connect(this, QNSIGNAL(FileSystemWatcherPrivate,fileRemoved,const QString&),
                     &m_parent, QNSIGNAL(FileSystemWatcher,fileRemoved,const QString&));

    QObject::connect(this, QNSIGNAL(FileSystemWatcherPrivate,directoryChanged,const QString&),
                     &m_parent, QNSIGNAL(FileSystemWatcher,directoryChanged,const QString&));
    QObject::connect(this, QNSIGNAL(FileSystemWatcherPrivate,directoryRemoved,const QString&),
                     &m_parent, QNSIGNAL(FileSystemWatcher,directoryRemoved,const QString&));

    QObject::connect(&m_watcher, QNSIGNAL(QFileSystemWatcher,fileChanged,const QString&),
                     this, QNSLOT(FileSystemWatcherPrivate,onFileChanged,const QString&));
    QObject::connect(&m_watcher, QNSIGNAL(QFileSystemWatcher,directoryChanged,const QString&),
                     this, QNSLOT(FileSystemWatcherPrivate,onDirectoryChanged,const QString&));
}

void FileSystemWatcherPrivate::processFileRemoval(const QString & path)
{
    PathWithTimerId::left_iterator it = m_justRemovedFilePathsWithPostRemovalTimerIds.left.find(path);
    if (it != m_justRemovedFilePathsWithPostRemovalTimerIds.left.end()) {
        return;
    }

    QNTRACE("It appears the watched file has been removed recently and no timer has been set up yet to track its removal; "
            "setting up such timer now");

    int timerId = startTimer(m_removalTimeoutMSec);
    m_justRemovedFilePathsWithPostRemovalTimerIds.insert(PathWithTimerId::value_type(path, timerId));
    QNTRACE("Set up timer with id " << timerId << " for " << m_removalTimeoutMSec << " to see if file " << path
            << " would re-appear again soon");
}

void FileSystemWatcherPrivate::processDirectoryRemoval(const QString &path)
{
    PathWithTimerId::left_iterator it = m_justRemovedDirectoryPathsWithPostRemovalTimerIds.left.find(path);
    if (it != m_justRemovedDirectoryPathsWithPostRemovalTimerIds.left.end()) {
        return;
    }

    QNTRACE("It appears the watched directory has been removed recently and no timer has been set up yet to track its removal; "
            "setting up such timer now");

    int timerId = startTimer(m_removalTimeoutMSec);
    m_justRemovedDirectoryPathsWithPostRemovalTimerIds.insert(PathWithTimerId::value_type(path, timerId));
    QNTRACE("Set up timer with id " << timerId << " for " << m_removalTimeoutMSec << " to see if directory "
            << path << " would re-appear again soon");
}

void FileSystemWatcherPrivate::timerEvent(QTimerEvent * pEvent)
{
    if (Q_UNLIKELY(!pEvent)) {
        return;
    }

    int timerId = pEvent->timerId();

    PathWithTimerId::right_iterator fileIt = m_justRemovedFilePathsWithPostRemovalTimerIds.right.find(timerId);
    if (fileIt != m_justRemovedFilePathsWithPostRemovalTimerIds.right.end())
    {
        const QString & filePath = fileIt->second;
        QFileInfo info(filePath);
        if (!info.isFile())
        {
            QNTRACE("File " << filePath << " doesn't exist after some time since its removal");

            auto it = m_watchedFilesWithDirs.left.find(filePath);
            if (it != m_watchedFilesWithDirs.left.end()) {
                Q_UNUSED(m_watchedFilesWithDirs.left.erase(it));
            }

            emit fileRemoved(filePath);
        }
        else
        {
            QNTRACE("File " << filePath << " exists again after some time since its removal");
            m_watcher.addPath(filePath);
            emit fileChanged(filePath);
        }

        Q_UNUSED(m_justRemovedFilePathsWithPostRemovalTimerIds.right.erase(fileIt));
        return;
    }

    // TODO: see if that was a timer for the directory
}

} // namespace qute_note
