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

#ifndef LIB_QUENTIER_TYPES_DATA_NOTE_DATA_H
#define LIB_QUENTIER_TYPES_DATA_NOTE_DATA_H

#include "FavoritableDataElementData.h"
#include <quentier/utility/QNLocalizedString.h>

#if QT_VERSION >= QT_VERSION_CHECK(5, 0, 0)
#include <qt5qevercloud/QEverCloud.h>
#else
#include <qt4qevercloud/QEverCloud.h>
#endif

#include <QImage>

namespace quentier {

class NoteData: public FavoritableDataElementData
{
public:
    NoteData();
    NoteData(const NoteData & other);
    NoteData(NoteData && other);
    NoteData(const qevercloud::Note & other);
    virtual ~NoteData();

    void clear();
    bool checkParameters(QNLocalizedString & errorDescription) const;

    QString plainText(QNLocalizedString * pErrorMessage) const;
    QStringList listOfWords(QNLocalizedString * pErrorMessage) const;
    std::pair<QString, QStringList> plainTextAndListOfWords(QNLocalizedString * pErrorMessage) const;

    bool containsToDoImpl(const bool checked) const;
    bool containsEncryption() const;

    void setContent(const QString & content);

    qevercloud::Note m_qecNote;

    struct ResourceAdditionalInfo
    {
        QString  localUid;
        bool     isDirty;

        bool operator==(const ResourceAdditionalInfo & other) const;
    };

    QList<ResourceAdditionalInfo>   m_resourcesAdditionalInfo;
    qevercloud::Optional<QString>   m_notebookLocalUid;
    QStringList                     m_tagLocalUids;
    QImage                          m_thumbnail;

private:
    NoteData & operator=(const NoteData & other) Q_DECL_EQ_DELETE;
    NoteData & operator=(NoteData && other) Q_DECL_EQ_DELETE;
};

} // namespace quentier

#endif // LIB_QUENTIER_TYPES_DATA_NOTE_DATA_H
