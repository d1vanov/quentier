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

#include "MainWindow.h"
#include <QApplication>
#include <QString>
#include <QDebug>

int main(int argc, char * argv[])
{
    QApplication app(argc, argv);

    if (argc < 4) {
        qWarning() << "Usage: " << argv[0]
                   << " <minidump location> "
                   << "<symbols file location> "
                   << "<stackwalker tool location>";
        return 1;
    }

    QString minidumpLocation = QString::fromUtf8(argv[1]);
    QString symbolsFileLocation = QString::fromUtf8(argv[2]);
    QString stackwalkBinaryLocation = QString::fromUtf8(argv[3]);

    MainWindow window(minidumpLocation,
                      symbolsFileLocation,
                      stackwalkBinaryLocation);
    window.show();

    return app.exec();
}
