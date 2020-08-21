/*
 * Copyright 2017-2020 Dmitry Ivanov
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

#ifndef QUENTIER_LIB_WIDGET_FILTER_BY_TAG_WIDGET_H
#define QUENTIER_LIB_WIDGET_FILTER_BY_TAG_WIDGET_H

#include "AbstractFilterByModelItemWidget.h"

#include <quentier/types/ErrorString.h>
#include <quentier/types/Tag.h>

#include <QPointer>
#include <QUuid>

namespace quentier {

QT_FORWARD_DECLARE_CLASS(LocalStorageManagerAsync)
QT_FORWARD_DECLARE_CLASS(TagModel)

class FilterByTagWidget : public AbstractFilterByModelItemWidget
{
    Q_OBJECT
public:
    explicit FilterByTagWidget(QWidget * parent = nullptr);

    virtual ~FilterByTagWidget() override;

    void setLocalStorageManager(
        LocalStorageManagerAsync & localStorageManagerAsync);

    const TagModel * tagModel() const;

private Q_SLOTS:
    void onUpdateTagCompleted(Tag tag, QUuid requestId);

    void onExpungeTagCompleted(
        Tag tag, QStringList expungedChildTagLocalUids, QUuid requestId);

    void onExpungeNotelessTagsFromLinkedNotebooksCompleted(QUuid requestId);

private:
    QPointer<LocalStorageManagerAsync> m_pLocalStorageManager;
};

} // namespace quentier

#endif // QUENTIER_LIB_WIDGET_FILTER_BY_TAG_WIDGET_H
