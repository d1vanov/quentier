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

#include <QByteArray>
#include <QCoreApplication>
#include <QStringList>
#include <QFile>
#include <QFileInfo>
#include <QDebug>

int main(int argc, char * argv[])
{
    QCoreApplication app(argc, argv);
    QStringList args = app.arguments();

    if (args.size() < 2) {
        qWarning() << QString::fromUtf8("Usage: ") << argv[0] << QString::fromUtf8(" ")
                   << QString::fromUtf8("<symbols file location>") << QString::fromUtf8(", args: ")
                   << QString::number(args.size());
        return 1;
    }

    QFile symbolsFile(args[1]);
    bool res = symbolsFile.open(QIODevice::ReadOnly);
    if (!res) {
        qWarning() << QString::fromUtf8("Can't open the symbols file for reading");
        return 1;
    }

    QByteArray fileContents = symbolsFile.readAll();
    symbolsFile.close();

    fileContents = qCompress(fileContents, 9);

    QFileInfo symbolsFileInfo(args[1]);
    QFile compressedSymbolsFile(symbolsFileInfo.absolutePath() + QString::fromUtf8("/") +
                                symbolsFileInfo.fileName() + QString::fromUtf8(".compressed"));
    res = compressedSymbolsFile.open(QIODevice::WriteOnly);
    if (!res) {
        qWarning() << QString::fromUtf8("Can't open the compressed symbols file for writing");
        return 1;
    }

    compressedSymbolsFile.write(fileContents);
    compressedSymbolsFile.close();

    return 0;
}
