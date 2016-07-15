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

#include "SpellCheckerDictionariesFinder.h"
#include <quentier/logging/QuentierLogger.h>
#include <QStringList>
#include <QFileInfo>
#include <QDirIterator>

namespace quentier {

#define WRAP(x) \
    << QString(x).toUpper()

static const QSet<QString> localeList = QSet<QString>()
#include "localeList.inl"
;

#undef WRAP

SpellCheckerDictionariesFinder::SpellCheckerDictionariesFinder(QObject * parent) :
    QObject(parent),
    m_files()
{}

void SpellCheckerDictionariesFinder::run()
{
    QNDEBUG("SpellCheckerDictionariesFinder::run");

    m_files.clear();
    QStringList fileFilters;
    fileFilters << "*.dic" << "*.aff";

    QFileInfoList rootDirs = QDir::drives();
    const int numRootDirs = rootDirs.size();
    for(int i = 0; i < numRootDirs; ++i)
    {
        const QFileInfo & rootDirInfo = rootDirs[i];

        if (Q_UNLIKELY(!rootDirInfo.isDir())) {
            QNTRACE("Skipping non-dir " << rootDirInfo.absoluteDir());
            continue;
        }

        QDirIterator it(rootDirInfo.absolutePath(), fileFilters, QDir::Files, QDirIterator::Subdirectories);
        while(it.hasNext())
        {
            QString nextDirName = it.next();
            QNTRACE("Next dir name = " << nextDirName);

            QFileInfo fileInfo = it.fileInfo();
            if (!fileInfo.isReadable()) {
                QNTRACE("Skipping non-readable file " << fileInfo.absoluteFilePath());
                continue;
            }

            QString fileNameSuffix = fileInfo.completeSuffix();
            bool isDicFile = false;
            if (fileNameSuffix == "dic") {
                isDicFile = true;
            }
            else if (fileNameSuffix != "aff") {
                QNTRACE("Skipping file not actually matching the filter: " << fileInfo.absoluteFilePath());
                continue;
            }

            QString dictionaryName = fileInfo.baseName();
            if (!localeList.contains(dictionaryName.toUpper())) {
                QNTRACE("Skipping dictionary which doesn't appear to correspond to any locale: " + dictionaryName);
                continue;
            }

            QPair<QString, QString> & pair = m_files[dictionaryName];
            if (isDicFile) {
                QNTRACE("Adding dic file " << fileInfo.absoluteFilePath());
                pair.first = fileInfo.absoluteFilePath();
            }
            else {
                QNTRACE("Adding aff file " << fileInfo.absoluteFilePath());
                pair.second = fileInfo.absoluteFilePath();
            }
        }
    }

    // Filter out any incomplete pair of dic & aff files
    for(auto it = m_files.begin(); it != m_files.end(); )
    {
        const QPair<QString, QString> & pair = it.value();
        if (pair.first.isEmpty() || pair.second.isEmpty()) {
            QNTRACE("Skipping the incomplete pair of dic/aff files: dic file path = "
                    << pair.first << "; aff file path = " << pair.second);
            it = m_files.erase(it);
            continue;
        }

        ++it;
    }

    QNDEBUG("Found " << m_files.size() << " valid dictionaries");
    emit foundDictionaries(m_files);
}

} // namespace quentier
