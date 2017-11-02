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

#include "AboutQuentierWidget.h"
#include "ui_AboutQuentierWidget.h"
#include <VersionInfo.h>
#include <quentier/utility/VersionInfo.h>

namespace quentier {

AboutQuentierWidget::AboutQuentierWidget(QWidget * parent) :
    QWidget(parent, Qt::Window),
    m_pUi(new Ui::AboutQuentierWidget)
{
    m_pUi->setupUi(this);

    m_pUi->quentierHeaderLabel->setText(QStringLiteral("Quentier ") +
                                        QStringLiteral(QUENTIER_MAJOR_VERSION) +
                                        QStringLiteral(".") +
                                        QStringLiteral(QUENTIER_MINOR_VERSION) +
                                        QStringLiteral(".") +
                                        QStringLiteral(QUENTIER_PATCH_VERSION));

    m_pUi->quentierQtVersionLabel->setText(QStringLiteral("Qt ") + QStringLiteral(QT_VERSION_STR));

    int libquentierVersion = quentier::libraryVersion();
    int libquentierMajorVersion = libquentierVersion / 10000;
    int libquentierMinorVersion = (libquentierVersion - libquentierMajorVersion * 10000) / 100;
    int libquentierPatchVersion = (libquentierVersion - libquentierMajorVersion * 10000 - libquentierMinorVersion * 100);

    QString libquentierVersionInfo = QStringLiteral("libquentier: ") +
                                     QStringLiteral(QUENTIER_LIBQUENTIER_BINARY_NAME) +
                                     QStringLiteral("; version") + QString::number(libquentierMajorVersion) +
                                     QStringLiteral(".") + QString::number(libquentierMinorVersion) +
                                     QStringLiteral(".") + QString::number(libquentierPatchVersion);

#if LIB_QUENTIER_USE_QT_WEB_ENGINE
    libquentierVersionInfo += QStringLiteral("; uses QtWebEngine");
#endif

    libquentierVersionInfo += QStringLiteral("; build info: ");
    libquentierVersionInfo += QStringLiteral(LIB_QUENTIER_BUILD_INFO);
    m_pUi->libquentierVersionLabel->setText(libquentierVersionInfo);

    m_pUi->quentierBuildVersionLabel->setText(QStringLiteral("Quentier build info: ") +
                                              QStringLiteral(QUENTIER_BUILD_INFO));
}

AboutQuentierWidget::~AboutQuentierWidget()
{
    delete m_pUi;
}

} // namespace quentier
