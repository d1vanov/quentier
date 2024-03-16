/*
 * Copyright 2017-2024 Dmitry Ivanov
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

#include "AboutQuentierWidget.h"
#include "ui_AboutQuentierWidget.h"

#include <lib/utility/HumanReadableVersionInfo.h>

#include <QtGlobal>

namespace quentier {

AboutQuentierWidget::AboutQuentierWidget(QWidget * parent) :
    QWidget{parent, Qt::Window}, m_ui{new Ui::AboutQuentierWidget}
{
    m_ui->setupUi(this);

    m_ui->quentierHeaderLabel->setText(quentierVersion());

    m_ui->quentierQtVersionLabel->setText(
        QStringLiteral("Built with Qt ") + QStringLiteral(QT_VERSION_STR) +
        QStringLiteral(", uses Qt ") + QString::fromUtf8(qVersion()));

    const QString libBuildInfo =
        QStringLiteral("Built with libquentier: ") + libquentierBuildTimeInfo();

    const QString libRuntimeInfo =
        QStringLiteral("Uses libquentier: ") + libquentierRuntimeInfo();

    m_ui->libquentierVersionLabel->setText(
        libBuildInfo + QStringLiteral("\n\n") + libRuntimeInfo);

    m_ui->quentierBuildVersionLabel->setText(
        QStringLiteral("Quentier build info: ") + quentierBuildInfo());
}

AboutQuentierWidget::~AboutQuentierWidget()
{
    delete m_ui;
}

} // namespace quentier
