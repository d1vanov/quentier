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

#ifndef LIB_QUENTIER_NOTE_EDITOR_JAVASCRIPT_GLUE_SPELL_CHECKER_DYNAMIC_HELPER_H
#define LIB_QUENTIER_NOTE_EDITOR_JAVASCRIPT_GLUE_SPELL_CHECKER_DYNAMIC_HELPER_H

#include <quentier/utility/Qt4Helper.h>
#include <QObject>
#include <QStringList>
#include <QVariant>

namespace quentier {

class SpellCheckerDynamicHelper: public QObject
{
    Q_OBJECT
public:
    explicit SpellCheckerDynamicHelper(QObject * parent = Q_NULLPTR);

Q_SIGNALS:
    void lastEnteredWords(QStringList words);

public Q_SLOTS:

    // NOTE: workarounding https://bugreports.qt.io/browse/QTBUG-39951 - JavaScript array doesn't get automatically converted to QVariant
#ifdef QUENTIER_USE_QT_WEB_ENGINE
    void setLastEnteredWords(QVariant words);
#else
    void setLastEnteredWords(QVariantList words);
#endif
};

} // namespace quentier

#endif // LIB_QUENTIER_NOTE_EDITOR_JAVASCRIPT_GLUE_SPELL_CHECKER_DYNAMIC_HELPER_H
