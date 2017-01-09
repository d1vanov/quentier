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

#ifndef QUENTIER_WIDGETS_FILTER_BY_NOTEBOOK_WIDGET_H
#define QUENTIER_WIDGETS_FILTER_BY_NOTEBOOK_WIDGET_H

#include "AbstractFilterByModelItemWidget.h"
#include <quentier/types/Notebook.h>
#include <quentier/utility/QNLocalizedString.h>
#include <QUuid>
#include <QPointer>

namespace quentier {

QT_FORWARD_DECLARE_CLASS(LocalStorageManagerThreadWorker)
QT_FORWARD_DECLARE_CLASS(NotebookModel)

class FilterByNotebookWidget: public AbstractFilterByModelItemWidget
{
    Q_OBJECT
public:
    explicit FilterByNotebookWidget(QWidget * parent = Q_NULLPTR);

    void setLocalStorageManager(LocalStorageManagerThreadWorker & localStorageManager);

    const NotebookModel * notebookModel() const;

private Q_SLOTS:
    void onUpdateNotebookCompleted(Notebook notebook, QUuid requestId);
    void onExpungeNotebookCompleted(Notebook notebook, QUuid requestId);

private:
    QPointer<LocalStorageManagerThreadWorker>   m_pLocalStorageManager;
};

} // namespace quentier

#endif // QUENTIER_WIDGETS_FILTER_BY_NOTEBOOK_WIDGET_H
