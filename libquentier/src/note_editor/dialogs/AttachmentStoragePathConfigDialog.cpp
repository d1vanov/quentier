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

#include "AttachmentStoragePathConfigDialog.h"
#include "ui_AttachmentStoragePathConfigDialog.h"
#include <quentier/utility/DesktopServices.h>
#include <quentier/utility/Qt4Helper.h>

namespace quentier {

AttachmentStoragePathConfigDialog::AttachmentStoragePathConfigDialog(QWidget * parent) :
    QDialog(parent, Qt::CustomizeWindowHint | Qt::WindowTitleHint | Qt::WindowMinMaxButtonsHint),
    m_pUI(new Ui::AttachmentStoragePathConfigDialog)
{
    m_pUI->setupUi(this);

    m_pUI->infoLabel->setText("<html><head/><body><p><span style=\" font-size:14pt; font-weight:600;\">" +
                              QApplication::applicationName() + " " +
                              tr("needs some folder to store copies of attachments. Unfortunately, "
                                 "the automatic guess for such folder has not found the appropriate path. "
                                 "Please provide such a path (or accept the home path as the default) "
                                 "and press OK. Note: the path needs to be writable by the application.") +
                              "</span></p></body></head></html>");
    m_pUI->infoLabel->setTextFormat(Qt::RichText);

    QString initialFolder = homePath();
    m_pUI->folderPath->setText(initialFolder);

    QFileInfo initialFolderInfo(initialFolder);
    m_pUI->buttonBox->button(QDialogButtonBox::Ok)->setEnabled(initialFolderInfo.exists() &&
                                                               initialFolderInfo.isDir() &&
                                                               initialFolderInfo.isWritable());
    QObject::connect(m_pUI->chooseFolderButton, QNSIGNAL(QPushButton,released),
                     this, QNSLOT(AttachmentStoragePathConfigDialog,onChooseFolder));
    QObject::connect(m_pUI->buttonBox->button(QDialogButtonBox::Ok), QNSIGNAL(QPushButton,released),
                     this, QNSLOT(AttachmentStoragePathConfigDialog,accept));
}

AttachmentStoragePathConfigDialog::~AttachmentStoragePathConfigDialog()
{
    delete m_pUI;
}

const QString AttachmentStoragePathConfigDialog::path() const
{
    return m_pUI->folderPath->text();
}

void AttachmentStoragePathConfigDialog::onChooseFolder()
{
    QString path = getExistingFolderDialog(this, tr("Choose folder to store attachments"),
                                           m_pUI->folderPath->text());
    m_pUI->folderPath->setText(path);

    QFileInfo pathInfo;
    m_pUI->buttonBox->button(QDialogButtonBox::Ok)->setEnabled(pathInfo.exists() &&
                                                               pathInfo.isDir() &&
                                                               pathInfo.isWritable());
}

const QString getAttachmentStoragePath(QWidget * parent)
{
    QScopedPointer<AttachmentStoragePathConfigDialog> pDialog(new AttachmentStoragePathConfigDialog(parent));
    if (parent) {
        pDialog->setWindowModality(Qt::WindowModal);
    }

    int res = pDialog->exec();
    if (res == QDialog::Accepted) {
        return pDialog->path();
    }
    else {
        return QString();
    }
}

} // namespace quentier
