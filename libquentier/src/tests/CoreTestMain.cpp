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

#include "CoreTester.h"
#include <quentier/logging/QuentierLogger.h>
#include <quentier/utility/QuentierApplication.h>
#include <QtTest/QtTest>
#include <QDebug>
#include <QSqlDatabase>

using namespace quentier::test;

int main(int argc, char *argv[])
{
    quentier::QuentierApplication app(argc, argv);
    app.setOrganizationName("d1vanov");
    app.setApplicationName("QuentierCoreTests");

    QUENTIER_INITIALIZE_LOGGING();
    QUENTIER_SET_MIN_LOG_LEVEL(Warn);

    QsLogging::Logger & logger = QsLogging::Logger::instance();
    logger.addDestination(QsLogging::DestinationFactory::MakeDebugOutputDestination());

    return QTest::qExec(new CoreTester);
}
