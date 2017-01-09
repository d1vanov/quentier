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

#ifndef LIB_QUENTIER_NOTE_EDITOR_SPELL_CHECKER_DICTIONARIES_FINDER_H
#define LIB_QUENTIER_NOTE_EDITOR_SPELL_CHECKER_DICTIONARIES_FINDER_H

#include <quentier/utility/Macros.h>
#include <QObject>
#include <QRunnable>
#include <QHash>
#include <QSet>
#include <QString>

namespace quentier {

class SpellCheckerDictionariesFinder: public QObject,
                                      public QRunnable
{
    Q_OBJECT
public:
    typedef QHash<QString, QPair<QString, QString> > DicAndAffFilesByDictionaryName;

public:
    SpellCheckerDictionariesFinder(QObject * parent = Q_NULLPTR);

    virtual void run() Q_DECL_OVERRIDE;

Q_SIGNALS:
    void foundDictionaries(DicAndAffFilesByDictionaryName docAndAffFilesByDictionaryName);

private:
    DicAndAffFilesByDictionaryName  m_files;
    const QSet<QString>             m_localeList;
};

} // namespace quentier

#endif // LIB_QUENTIER_NOTE_EDITOR_SPELL_CHECKER_DICTIONARIES_FINDER_H
