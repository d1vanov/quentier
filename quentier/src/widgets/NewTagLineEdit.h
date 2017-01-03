/*
 * Copyright 2016 Dmitry Ivanov
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

#ifndef QUENTIER_WIDGETS_NEW_TAG_LINE_EDIT_H
#define QUENTIER_WIDGETS_NEW_TAG_LINE_EDIT_H

#include <quentier/utility/Qt4Helper.h>
#include <QLineEdit>

namespace Ui {
class NewTagLineEdit;
}

QT_FORWARD_DECLARE_CLASS(QCompleter)

namespace quentier {

QT_FORWARD_DECLARE_CLASS(TagModel)

class NewTagLineEdit: public QLineEdit
{
    Q_OBJECT
public:
    explicit NewTagLineEdit(TagModel * pTagModel, QWidget * parent = Q_NULLPTR);
    virtual ~NewTagLineEdit();

private Q_SLOTS:
    void onTagModelSortingChanged();

protected:
    virtual void keyPressEvent(QKeyEvent * pEvent) Q_DECL_OVERRIDE;

private:
    void setupCompleter();

private:
    Ui::NewTagLineEdit *    m_pUi;
    TagModel *              m_pTagModel;
    QCompleter *            m_pCompleter;
};

} // namespace quentier

#endif // QUENTIER_WIDGETS_NEW_TAG_LINE_EDIT_H
