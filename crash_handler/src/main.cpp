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
#include <QStringList>
#include <QDebug>

int main(int argc, char * argv[])
{
    QApplication app(argc, argv);

    QStringList args = app.arguments();
    if (args.size() < 4) {
        qWarning() << QString::fromUtf8("Usage: ") << argv[0] << QString::fromUtf8(" ")
                   << QString::fromUtf8("<symbols file location> <stackwalker tool location> <minidump file location>");
        return 1;
    }

    MainWindow window(args.at(1), args.at(2), args.at(3));
    window.show();

    return app.exec();
}
