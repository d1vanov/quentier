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

#ifndef QUENTIER_MODELS_SAVED_SEARCH_MODEL_ITEM_H
#define QUENTIER_MODELS_SAVED_SEARCH_MODEL_ITEM_H

#include <quentier/utility/Printable.h>

namespace quentier {

class SavedSearchModelItem: public Printable
{
public:
    explicit SavedSearchModelItem(const QString & localUid = QString(),
                                  const QString & guid = QString(),
                                  const QString & name = QString(),
                                  const QString & query = QString(),
                                  const bool isSynchronizable = false,
                                  const bool isDirty = false,
                                  const bool isFavorited = false);

    virtual QTextStream & print(QTextStream & strm) const Q_DECL_OVERRIDE;

    QString nameUpper() const { return m_name.toUpper(); }

    QString     m_localUid;
    QString     m_guid;
    QString     m_name;
    QString     m_query;
    bool        m_isSynchronizable;
    bool        m_isDirty;
    bool        m_isFavorited;
};

} // namespace quentier

#endif // QUENTIER_MODELS_SAVED_SEARCH_MODEL_ITEM_H
