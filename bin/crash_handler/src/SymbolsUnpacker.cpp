/*
 * Copyright 2017-2020 Dmitry Ivanov
 *
 * This file is part of Quentier.
 *
 * Quentier is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, version 3 of the License.
 *
 * Quentier is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with Quentier. If not, see <http://www.gnu.org/licenses/>.
 */

#include "SymbolsUnpacker.h"
#include "Utility.h"

#include <VersionInfo.h>

#include <quentier/utility/Utility.h>
#include <quentier/utility/VersionInfo.h>

#include <QDir>
#include <QFile>
#include <QFileInfo>
#include <QRegExp>
#include <QTemporaryFile>

#include <cstring>
#include <fstream>
#include <iostream>

SymbolsUnpacker::SymbolsUnpacker(
    const QString & compressedSymbolsFilePath,
    const QString & unpackedSymbolsRootPath, QObject * parent) :
    QObject(parent),
    m_compressedSymbolsFilePath(compressedSymbolsFilePath),
    m_unpackedSymbolsRootPath(unpackedSymbolsRootPath)
{}

SymbolsUnpacker::~SymbolsUnpacker() = default;

void SymbolsUnpacker::run()
{
    // 1) Verify the root path for the unpacked symbols is good - it exists, it
    // is a dir and it is writable

    QString unpackedSymbolsRootPath =
        nativePathToUnixPath(m_unpackedSymbolsRootPath);

    QFileInfo unpackedSymbolsRootDirInfo(unpackedSymbolsRootPath);
    if (Q_UNLIKELY(!unpackedSymbolsRootDirInfo.exists())) {
        QDir unpackedSymbolsRootDir(unpackedSymbolsRootPath);
        if (!unpackedSymbolsRootDir.mkpath(unpackedSymbolsRootPath)) {
            QString errorDescription =
                tr("Error: can't create the directory for the unpacked symbols "
                   "file") +
                QStringLiteral(": ") +
                QDir::toNativeSeparators(unpackedSymbolsRootPath);

            Q_EMIT finished(/* status = */ false, errorDescription);
            return;
        }
    }
    else if (Q_UNLIKELY(!unpackedSymbolsRootDirInfo.isDir())) {
        QString errorDescription =
            tr("Error: the path to the directory for the unpacked symbols file "
               "doesn't really point to a directory") +
            QStringLiteral(": ") +
            QDir::toNativeSeparators(unpackedSymbolsRootPath);

        Q_EMIT finished(/* status = */ false, errorDescription);
        return;
    }
    else if (Q_UNLIKELY(!unpackedSymbolsRootDirInfo.isWritable())) {
        QString errorDescription =
            tr("Error: the directory for the unpacked symbols is not "
               "writable") +
            QStringLiteral(": ") +
            QDir::toNativeSeparators(unpackedSymbolsRootPath);

        Q_EMIT finished(/* status = */ false, errorDescription);
        return;
    }

    // 2) Read the data from the compressed symbols file

    QString compressedSymbolsFilePath =
        nativePathToUnixPath(m_compressedSymbolsFilePath);

    QFileInfo compressedSymbolsFileInfo(compressedSymbolsFilePath);

    if (Q_UNLIKELY(!compressedSymbolsFileInfo.exists())) {
        QString errorDescription =
            tr("Error: compressed symbols file doesn't exist") +
            QStringLiteral(": ") +
            QDir::toNativeSeparators(compressedSymbolsFilePath);

        Q_EMIT finished(/* status = */ false, errorDescription);
        return;
    }

    if (Q_UNLIKELY(!compressedSymbolsFileInfo.isFile())) {
        QString errorDescription =
            tr("Error: the path to symbols file doesn't really point to "
               "a file") +
            QStringLiteral(": ") +
            QDir::toNativeSeparators(compressedSymbolsFilePath);

        Q_EMIT finished(/* status = */ false, errorDescription);
        return;
    }

    QFile compressedSymbolsFile(compressedSymbolsFileInfo.absoluteFilePath());
    if (!compressedSymbolsFile.open(QIODevice::ReadOnly)) {
        QString errorDescription =
            tr("Error: can't open the compressed symbols file for reading") +
            QStringLiteral(": ") +
            QDir::toNativeSeparators(compressedSymbolsFilePath);

        Q_EMIT finished(/* status = */ false, errorDescription);
        return;
    }

    QByteArray symbolsUncompressedData = compressedSymbolsFile.readAll();
    compressedSymbolsFile.close();

    // 3) Uncompress the symbols data and write them into a temporary file

    QTemporaryFile uncompressedSymbolsFile;
    if (!uncompressedSymbolsFile.open()) {
        QString errorDescription =
            tr("Error: can't create the temporary file to store "
               "the uncompressed symbols read from the compressed file");

        Q_EMIT finished(/* status = */ false, errorDescription);
        return;
    }

    uncompressedSymbolsFile.setAutoRemove(true);

    symbolsUncompressedData = qUncompress(symbolsUncompressedData);
    uncompressedSymbolsFile.write(symbolsUncompressedData);

    // 4) Read the first line from the uncompressed symbols file and use that
    // data to identify the name of the symbols source as well as its id

    QFileInfo uncompressedSymbolsFileInfo(uncompressedSymbolsFile);

    // NOTE: all attempts to read the first line from the symbols file using
    // QFile's API result in crash for unknown reason; maybe it doesn't like
    // what it finds within the file, I have no idea.
    // Working around this crap by using C++ standard library's API.
    std::ifstream symbolsFileStream;

    symbolsFileStream.open(
        QDir::toNativeSeparators(uncompressedSymbolsFileInfo.absoluteFilePath())
            .toLocal8Bit()
            .constData(),
        std::ifstream::in);

    if (Q_UNLIKELY(!symbolsFileStream.good())) {
        QString errorDescription =
            tr("Error: can't open the temporary uncompressed symbols file for "
               "reading") +
            QStringLiteral(": ") +
            QDir::toNativeSeparators(
                uncompressedSymbolsFileInfo.absoluteFilePath());

        Q_EMIT finished(/* status = */ false, errorDescription);
        return;
    }

    char buf[1024];
    symbolsFileStream.getline(buf, 1024);

    QByteArray symbolsFirstLineBytes(buf, 1024);
    QString symbolsFirstLine = QString::fromUtf8(symbolsFirstLineBytes);
    QString symbolsSourceName = compressedSymbolsFileInfo.fileName();

    int suffixIndex =
        symbolsSourceName.indexOf(QStringLiteral(".syms.compressed"));

    if (suffixIndex >= 0) {
        symbolsSourceName.truncate(suffixIndex);
    }

#ifdef _MSC_VER
    int dllIndex = symbolsSourceName.indexOf(QStringLiteral(".dll"));
    if (dllIndex >= 0) {
        symbolsSourceName.truncate(dllIndex);
    }
#endif

    int symbolsSourceNameIndex = symbolsFirstLine.indexOf(symbolsSourceName);
    if (Q_UNLIKELY(symbolsSourceNameIndex < 0)) {
        QString errorDescription =
            tr("Error: can't find the symbols source name hint") +
            QStringLiteral(" \"") + symbolsSourceName + QStringLiteral("\" ") +
            tr("within the first 1024 bytes read from the symbols file") +
            QStringLiteral(": ") + QString::fromLocal8Bit(buf, 1024);

        Q_EMIT finished(/* status = */ false, errorDescription);
        return;
    }

    symbolsFirstLine.truncate(
        symbolsSourceNameIndex + symbolsSourceName.size());

    QRegExp regex(QStringLiteral("\\s"));
    QStringList symbolsFirstLineTokens = symbolsFirstLine.split(regex);

    if (symbolsFirstLineTokens.size() != 5) {
        QString errorDescription =
            tr("Error: unexpected number of tokens at the first line of "
               "the symbols file") +
            QStringLiteral(": ") +
            symbolsFirstLineTokens.join(QStringLiteral(", "));

        Q_EMIT finished(/* status = */ false, errorDescription);
        return;
    }

    QString symbolsId = symbolsFirstLineTokens.at(3);
    if (Q_UNLIKELY(symbolsId.isEmpty())) {
        QString errorDescription =
            tr("Error: symbol id is empty, first line of the minidump file") +
            QStringLiteral(": ") +
            symbolsFirstLineTokens.join(QStringLiteral(", "));

        Q_EMIT finished(/* status = */ false, errorDescription);
        return;
    }

    symbolsSourceName = symbolsFirstLineTokens.at(4);
    if (Q_UNLIKELY(symbolsSourceName.isEmpty())) {
        QString errorDescription =
            tr("Error: minidump's application name is empty, first line of "
               "the minidump file") +
            QStringLiteral(": ") +
            symbolsFirstLineTokens.join(QStringLiteral(", "));

        Q_EMIT finished(/* status = */ false, errorDescription);
        return;
    }

#ifdef Q_OS_LINUX
    if (symbolsSourceName == QStringLiteral("quentier")) {
        symbolsSourceName += QStringLiteral("-");
        symbolsSourceName += QString::fromUtf8(QUENTIER_MAJOR_VERSION);
        symbolsSourceName += QStringLiteral(".");
        symbolsSourceName += QString::fromUtf8(QUENTIER_MINOR_VERSION);
        symbolsSourceName += QStringLiteral(".");
        symbolsSourceName += QString::fromUtf8(QUENTIER_PATCH_VERSION);
    }
#endif

#if defined(_MSC_VER)
    symbolsSourceName += QStringLiteral(".pdb");
#else
    if (symbolsSourceName.contains(QStringLiteral("libquentier")) ||
        symbolsSourceName.contains(QStringLiteral("libqt5quentier")))
    {
        if (QUENTIER_PACKAGED_AS_APP_IMAGE) {
            symbolsSourceName = QStringLiteral("libqt5quentier.so.");
            symbolsSourceName += QString::number(LIB_QUENTIER_VERSION_MAJOR);
        }
        else {
            symbolsSourceName =
                QString::fromUtf8(QUENTIER_LIBQUENTIER_BINARY_NAME);
        }
    }
#endif

    QString unpackDirPath = unpackedSymbolsRootPath + QStringLiteral("/") +
        symbolsSourceName + QStringLiteral("/") + symbolsId;

    bool res = quentier::removeDir(unpackDirPath);
    if (Q_UNLIKELY(!res)) {
        QString errorDescription =
            tr("Error: the temporary directory for unpacked symbols already "
               "exists and it can't be removed") +
            QStringLiteral(":\n") + QDir::toNativeSeparators(unpackDirPath);

        Q_EMIT finished(/* status = */ false, errorDescription);
        return;
    }

    QDir unpackDir(unpackDirPath);
    res = unpackDir.mkpath(unpackDirPath);
    if (Q_UNLIKELY(!res)) {
        QString errorDescription =
            tr("Error: failed to create the temporary directory for unpacking "
               "the symbols") +
            QStringLiteral(":\n") + QDir::toNativeSeparators(unpackDirPath);

        Q_EMIT finished(/* status = */ false, errorDescription);
        return;
    }

#ifdef _MSC_VER
    int pdbIndex = symbolsSourceName.indexOf(QStringLiteral(".pdb"));
    if (pdbIndex > 0) {
        symbolsSourceName.truncate(pdbIndex);
    }
#endif

    QString newSymbolsFilePath = unpackDirPath + QStringLiteral("/") +
        symbolsSourceName + QStringLiteral(".sym");

    QFile newSymbolsFile(newSymbolsFilePath);
    res = newSymbolsFile.open(QIODevice::WriteOnly);
    if (Q_UNLIKELY(!res)) {
        QString errorDescription =
            tr("Error: failed to open the temporary file for unpacked symbols "
               "for writing") +
            QStringLiteral(":\n") + QDir::toNativeSeparators(unpackDirPath);

        Q_EMIT finished(/* status = */ false, errorDescription);
        return;
    }

#ifndef _MSC_VER
    // Need to replace the first line within the uncompressed data to ensure
    // the proper names used
    int firstLineBreakIndex = symbolsUncompressedData.indexOf('\n');
    if (firstLineBreakIndex > 0) {
        QString replacementFirstLine = QStringLiteral("MODULE ");
        replacementFirstLine += symbolsFirstLineTokens[1];
        replacementFirstLine += QStringLiteral(" ");
        replacementFirstLine += symbolsFirstLineTokens[2];
        replacementFirstLine += QStringLiteral(" ");
        replacementFirstLine += symbolsId;
        replacementFirstLine += QStringLiteral(" ");
        replacementFirstLine += symbolsSourceName;
        replacementFirstLine += QStringLiteral("\n");

        symbolsUncompressedData.replace(
            0, firstLineBreakIndex, replacementFirstLine.toLocal8Bit());
    }
#endif

    newSymbolsFile.write(symbolsUncompressedData);
    newSymbolsFile.close();

    emit finished(/* status = */ true, QString());
}
