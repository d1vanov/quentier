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

#ifndef LIB_QUENTIER_NOTE_EDITOR_ATTACHMENT_STORAGE_PATH_CONFIG_DIALOG_H
#define LIB_QUENTIER_NOTE_EDITOR_ATTACHMENT_STORAGE_PATH_CONFIG_DIALOG_H

#include <quentier/utility/Qt4Helper.h>
#include <QDialog>

namespace Ui {
QT_FORWARD_DECLARE_CLASS(AttachmentStoragePathConfigDialog)
}

namespace quentier {

class AttachmentStoragePathConfigDialog: public QDialog
{
    Q_OBJECT
public:
    explicit AttachmentStoragePathConfigDialog(QWidget * parent = Q_NULLPTR);
    virtual ~AttachmentStoragePathConfigDialog();

    const QString path() const;

private Q_SLOTS:
    void onChooseFolder();

private:
    Ui::AttachmentStoragePathConfigDialog * m_pUI;
};

const QString getAttachmentStoragePath(QWidget * parent);

} // namespace quentier

#endif // LIB_QUENTIER_NOTE_EDITOR_ATTACHMENT_STORAGE_PATH_CONFIG_DIALOG_H
