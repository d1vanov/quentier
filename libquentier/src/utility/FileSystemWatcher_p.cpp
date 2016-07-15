/*
 * Copyright 2016 Dmitry Ivanov
 *
 * This file is part of libquentier
 *
 * libquentier is free software; you can redistribute it and/or modify
 * it under the terms of the GNU Lesser General Public License as published by
 * the Free Software Foundation, version 3 of the License.
 *
 * libquentier is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU Lesser General Public License for more details.
 *
 * You should have received a copy of the GNU Lesser General Public License
 * along with libquentier. If not, see <http://www.gnu.org/licenses/>.
 */

#include "FileSystemWatcher_p.h"
#include <quentier/logging/QuentierLogger.h>
#include <QFileInfo>
#include <QTimerEvent>

namespace quentier {

FileSystemWatcherPrivate::FileSystemWatcherPrivate(FileSystemWatcher & parent, const int removalTimeoutMSec) :
    QObject(&parent),
    m_parent(parent),
    m_watcher(),
    m_removalTimeoutMSec(removalTimeoutMSec),
    m_watchedFiles(),
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
    m_watchedFiles(),
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
        Q_UNUSED(m_watchedFiles.insert(path));
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
    auto fileIt = m_watchedFiles.find(path);
    if (fileIt != m_watchedFiles.end()) {
        Q_UNUSED(m_watchedFiles.erase(fileIt));
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

    auto fileIt = m_watchedFiles.find(path);
    if (Q_UNLIKELY(fileIt == m_watchedFiles.end())) {
        QNWARNING("Received file changed event for file not listed as watched");
        return;
    }

    QFileInfo info(path);
    if (!info.isFile()) {
        processFileRemoval(path);
    }
    else {
        m_watcher.addPath(path);
        emit fileChanged(path);
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
            m_watcher.addPath(path);
            emit directoryChanged(path);
        }
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
    killTimer(timerId);

    PathWithTimerId::right_iterator fileIt = m_justRemovedFilePathsWithPostRemovalTimerIds.right.find(timerId);
    if (fileIt != m_justRemovedFilePathsWithPostRemovalTimerIds.right.end())
    {
        const QString & filePath = fileIt->second;
        QFileInfo info(filePath);
        if (!info.isFile())
        {
            QNTRACE("File " << filePath << " doesn't exist after some time since its removal");

            auto it = m_watchedFiles.find(filePath);
            if (it != m_watchedFiles.end()) {
                Q_UNUSED(m_watchedFiles.erase(it));
                emit fileRemoved(filePath);
            }
        }
        else
        {
            QNTRACE("File " << filePath << " exists again after some time since its removal");
            auto it = m_watchedFiles.find(filePath);
            if (it != m_watchedFiles.end()) {
                m_watcher.addPath(filePath);
                emit fileChanged(filePath);
            }
        }

        Q_UNUSED(m_justRemovedFilePathsWithPostRemovalTimerIds.right.erase(fileIt));
        return;
    }

    PathWithTimerId::right_iterator dirIt = m_justRemovedDirectoryPathsWithPostRemovalTimerIds.right.find(timerId);
    if (dirIt != m_justRemovedDirectoryPathsWithPostRemovalTimerIds.right.end())
    {
        const QString & directoryPath = dirIt->second;
        QFileInfo info(directoryPath);
        if (!info.isDir())
        {
            QNTRACE("Directory " << directoryPath << " doesn't exist after some time since its removal");

            auto it = m_watchedDirectories.find(directoryPath);
            if (it != m_watchedDirectories.end()) {
                Q_UNUSED(m_watchedDirectories.erase(it));
                emit directoryRemoved(directoryPath);
            }
        }
        else
        {
            QNTRACE("Directory " << directoryPath << " exists again after some time since its removal");
            auto it = m_watchedDirectories.find(directoryPath);
            if (it != m_watchedDirectories.end()) {
                m_watcher.addPath(directoryPath);
                emit directoryChanged(directoryPath);
            }
        }

        Q_UNUSED(m_justRemovedDirectoryPathsWithPostRemovalTimerIds.right.erase(dirIt));
        return;
    }
}

} // namespace quentier
