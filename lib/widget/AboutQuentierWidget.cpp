/*
 * Copyright 2017-2021 Dmitry Ivanov
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
    QWidget(parent, Qt::Window), m_pUi(new Ui::AboutQuentierWidget)
{
    m_pUi->setupUi(this);

    m_pUi->quentierHeaderLabel->setText(quentierVersion());

    m_pUi->quentierQtVersionLabel->setText(
        QStringLiteral("Built with Qt ") + QStringLiteral(QT_VERSION_STR) +
        QStringLiteral(", uses Qt ") + QString::fromUtf8(qVersion()));

    const QString libBuildInfo =
        QStringLiteral("Built with libquentier: ") + libquentierBuildTimeInfo();

    const QString libRuntimeInfo =
        QStringLiteral("Uses libquentier: ") + libquentierRuntimeInfo();

    m_pUi->libquentierVersionLabel->setText(
        libBuildInfo + QStringLiteral("\n\n") + libRuntimeInfo);

    m_pUi->quentierBuildVersionLabel->setText(
        QStringLiteral("Quentier build info: ") + quentierBuildInfo());
}

AboutQuentierWidget::~AboutQuentierWidget()
{
    delete m_pUi;
}

} // namespace quentier
