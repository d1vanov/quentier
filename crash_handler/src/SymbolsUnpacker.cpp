/*
 * Copyright 2017 Dmitry Ivanov
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
#include <PackagingInfo.h>
#include <QFileInfo>
#include <QFile>
#include <QTemporaryFile>
#include <QDir>
#include <QRegExp>
#include <iostream>
#include <fstream>
#include <cstring>

SymbolsUnpacker::SymbolsUnpacker(const QString & compressedSymbolsFilePath,
                                 const QString & unpackedSymbolsRootPath,
                                 QObject * parent) :
    QObject(parent),
    m_compressedSymbolsFilePath(compressedSymbolsFilePath),
    m_unpackedSymbolsRootPath(unpackedSymbolsRootPath)
{}

void SymbolsUnpacker::run()
{
    // 1) Verify the root path for the unpacked symbols is good - it exists, it
    // is a dir and it is writable

    QString unpackedSymbolsRootPath = nativePathToUnixPath(m_unpackedSymbolsRootPath);
    QFileInfo unpackedSymbolsRootDirInfo(unpackedSymbolsRootPath);
    if (Q_UNLIKELY(!unpackedSymbolsRootDirInfo.exists()))
    {
        QDir unpackedSymbolsRootDir(unpackedSymbolsRootPath);
        if (!unpackedSymbolsRootDir.mkpath(unpackedSymbolsRootPath)) {
            QString errorDescription = tr("Error: can't create the directory for the unpacked symbols file") +
                                       QString::fromUtf8(": ") + QDir::toNativeSeparators(unpackedSymbolsRootPath);
            emit finished(/* status = */ false, errorDescription);
            return;
        }
    }
    else if (Q_UNLIKELY(!unpackedSymbolsRootDirInfo.isDir()))
    {
        QString errorDescription = tr("Error: the path to the directory for the unpacked symbols file doesn't really point to a directory") +
                                       QString::fromUtf8(": ") + QDir::toNativeSeparators(unpackedSymbolsRootPath);
        emit finished(/* status = */ false, errorDescription);
        return;
    }
    else if (Q_UNLIKELY(!unpackedSymbolsRootDirInfo.isWritable()))
    {
        QString errorDescription = tr("Error: the directory for the unpacked symbols is not writable") +
            QString::fromUtf8(": ") + QDir::toNativeSeparators(unpackedSymbolsRootPath);
        emit finished(/* status = */ false, errorDescription);
        return;
    }

    // 2) Read the data from the compressed symbols file

    QString compressedSymbolsFilePath = nativePathToUnixPath(m_compressedSymbolsFilePath);
    QFileInfo compressedSymbolsFileInfo(compressedSymbolsFilePath);

    if (Q_UNLIKELY(!compressedSymbolsFileInfo.exists())) {
        QString errorDescription = tr("Error: compressed symbols file doesn't exist") + QString::fromUtf8(": ") +
                                   QDir::toNativeSeparators(compressedSymbolsFilePath);
        emit finished(/* status = */ false, errorDescription);
        return;
    }

    if (Q_UNLIKELY(!compressedSymbolsFileInfo.isFile())) {
        QString errorDescription = tr("Error: the path to symbols file doesn't really point to a file") +
                                   QString::fromUtf8(": ") + QDir::toNativeSeparators(compressedSymbolsFilePath);
        emit finished(/* status = */ false, errorDescription);
        return;
    }

    QFile compressedSymbolsFile(compressedSymbolsFileInfo.absoluteFilePath());
    if (!compressedSymbolsFile.open(QIODevice::ReadOnly)) {
        QString errorDescription = tr("Error: can't open the compressed symbols file for reading") +
                                   QString::fromUtf8(": ") + QDir::toNativeSeparators(compressedSymbolsFilePath);
        emit finished(/* status = */ false, errorDescription);
        return;
    }

    QByteArray symbolsUncompressedData = compressedSymbolsFile.readAll();
    compressedSymbolsFile.close();

    // 3) Uncompress the symbols data and write them into a temporary file

    QTemporaryFile uncompressedSymbolsFile;
    if (!uncompressedSymbolsFile.open()) {
        QString errorDescription = tr("Error: can't create the temporary file to store the uncompressed symbols read from the compressed file");
        emit finished(/* status = */ false, errorDescription);
        return;
    }

    uncompressedSymbolsFile.setAutoRemove(true);

    symbolsUncompressedData = qUncompress(symbolsUncompressedData);
    uncompressedSymbolsFile.write(symbolsUncompressedData);

    // 4) Read the first line from the uncompressed symbols file and use that data
    // to identify the name of the symbols source as well as its id

    QFileInfo uncompressedSymbolsFileInfo(uncompressedSymbolsFile);

    // NOTE: all attempts to read the first line from the symbols file using QFile's API result in crash for unknown reason;
    // maybe it doesn't like what it finds within the file, I have no idea.
    // Working around this crap by using C++ standard library's API.
    std::ifstream symbolsFileStream;
    symbolsFileStream.open(QDir::toNativeSeparators(uncompressedSymbolsFileInfo.absoluteFilePath()).toLocal8Bit().constData(), std::ifstream::in);
    if (Q_UNLIKELY(!symbolsFileStream.good())) {
        QString errorDescription = tr("Error: can't open the temporary uncompressed symbols file for reading") +
                                   QString::fromUtf8(": ") + QDir::toNativeSeparators(uncompressedSymbolsFileInfo.absoluteFilePath());
        emit finished(/* status = */ false, errorDescription);
        return;
    }

    char buf[1024];
    symbolsFileStream.getline(buf, 1024);

    QByteArray symbolsFirstLineBytes(buf, 1024);
    QString symbolsFirstLine = QString::fromUtf8(symbolsFirstLineBytes);
    QString symbolsSourceName = compressedSymbolsFileInfo.fileName();
    int suffixIndex = symbolsSourceName.indexOf(QString::fromUtf8(".syms.compressed"));
    if (suffixIndex >= 0) {
        symbolsSourceName.truncate(suffixIndex);
    }

#ifdef _MSC_VER
    int dllIndex = symbolsSourceName.indexOf(QString::fromUtf8(".dll"));
    if (dllIndex >= 0) {
        symbolsSourceName.truncate(dllIndex);
    }
#endif

    int symbolsSourceNameIndex = symbolsFirstLine.indexOf(symbolsSourceName);
    if (Q_UNLIKELY(symbolsSourceNameIndex < 0)) {
        QString errorDescription = tr("Error: can't find the symbols source name hint") +
                                   QString::fromUtf8(" \"") + symbolsSourceName + QString::fromUtf8("\" ") +
                                   tr("within the first 1024 bytes read from the symbols file") +
                                   QString::fromUtf8(": ") + QString::fromLocal8Bit(buf, 1024);
        emit finished(/* status = */ false, errorDescription);
        return;
    }

    symbolsFirstLine.truncate(symbolsSourceNameIndex + symbolsSourceName.size());

    QRegExp regex(QString::fromUtf8("\\s"));
    QStringList symbolsFirstLineTokens = symbolsFirstLine.split(regex);

    if (symbolsFirstLineTokens.size() != 5) {
        QString errorDescription = tr("Error: unexpected number of tokens at the first line of the symbols file") +
                                   QString::fromUtf8(": ") + symbolsFirstLineTokens.join(QString::fromUtf8(", "));
        emit finished(/* status = */ false, errorDescription);
        return;
    }

    QString symbolsId = symbolsFirstLineTokens.at(3);
    if (Q_UNLIKELY(symbolsId.isEmpty())) {
        QString errorDescription = tr("Error: symbol id is empty, first line of the minidump file") +
                                   QString::fromUtf8(": ") + symbolsFirstLineTokens.join(QString::fromUtf8(", "));
        emit finished(/* status = */ false, errorDescription);
        return;
    }

    symbolsSourceName = symbolsFirstLineTokens.at(4);
    if (Q_UNLIKELY(symbolsSourceName.isEmpty())) {
        QString errorDescription = tr("Error: minidump's application name is empty, first line of the minidump file") +
                                   QString::fromUtf8(": ") + symbolsFirstLineTokens.join(QString::fromUtf8(", "));
        emit finished(/* status = */ false, errorDescription);
        return;
    }

#if defined(_MSC_VER)
    symbolsSourceName += QString::fromUtf8(".pdb");
#else
    if (symbolsSourceName.contains(QStringLiteral("lib"))) {
        symbolsSourceName = QString::fromUtf8(QUENTIER_LIBQUENTIER_BINARY_NAME);
    }
    else if (QUENTIER_PACKAGED_AS_APP_IMAGE) {
        // NOTE: when AppImage-packaged quentier crashes, the symbols id of the executable is composed of all-zeros,
        // presumably due to crashed binary being AppRun, the tiny file
        symbolsId = QStringLiteral("000000000000000000000000000000000");
    }
#endif

    QString unpackDirPath = unpackedSymbolsRootPath + QString::fromUtf8("/") +
                            symbolsSourceName + QString::fromUtf8("/") + symbolsId;

    bool res = removeDir(unpackDirPath);
    if (Q_UNLIKELY(!res)) {
        QString errorDescription = tr("Error: the temporary directory for unpacked symbols already exists and it can't be removed") +
                                   QString::fromUtf8(":\n") + QDir::toNativeSeparators(unpackDirPath);
        emit finished(/* status = */ false, errorDescription);
        return;
    }

    QDir unpackDir(unpackDirPath);
    res = unpackDir.mkpath(unpackDirPath);
    if (Q_UNLIKELY(!res)) {
        QString errorDescription = tr("Error: failed to create the temporary directory for unpacking the symbols") +
                                   QString::fromUtf8(":\n") + QDir::toNativeSeparators(unpackDirPath);
        emit finished(/* status = */ false, errorDescription);
        return;
    }

#ifdef _MSC_VER
    int pdbIndex = symbolsSourceName.indexOf(QString::fromUtf8(".pdb"));
    if (pdbIndex > 0) {
        symbolsSourceName.truncate(pdbIndex);
    }
#endif

    QString newSymbolsFilePath = unpackDirPath + QString::fromUtf8("/") + symbolsSourceName + QString::fromUtf8(".sym");

    QFile newSymbolsFile(newSymbolsFilePath);
    res = newSymbolsFile.open(QIODevice::WriteOnly);
    if (Q_UNLIKELY(!res)) {
        QString errorDescription = tr("Error: failed to open the temporary file for unpacked symbols for writing") +
                                   QString::fromUtf8(":\n") + QDir::toNativeSeparators(unpackDirPath);
        emit finished(/* status = */ false, errorDescription);
        return;
    }

#ifndef _MSC_VER
    // Need to replace the first line within the uncompressed data to ensure the proper names used
    int firstLineLength = static_cast<int>(strlen(buf));
    int uncompressedSymbolsSize = symbolsUncompressedData.size();
    if (uncompressedSymbolsSize > firstLineLength) {
        QString replacementFirstLine = QStringLiteral("MODULE ");
        replacementFirstLine += symbolsFirstLineTokens[1];
        replacementFirstLine += QStringLiteral(" ");
        replacementFirstLine += symbolsFirstLineTokens[2];
        replacementFirstLine += QStringLiteral(" ");
        replacementFirstLine += symbolsId;
        replacementFirstLine += QStringLiteral(" ");
        replacementFirstLine += symbolsSourceName;
        replacementFirstLine += QStringLiteral("\n");
        symbolsUncompressedData.replace(0, uncompressedSymbolsSize - firstLineLength, replacementFirstLine.toLocal8Bit());
    }
#endif

    newSymbolsFile.write(symbolsUncompressedData);
    newSymbolsFile.close();

    emit finished(/* status = */ true, QString());
}
