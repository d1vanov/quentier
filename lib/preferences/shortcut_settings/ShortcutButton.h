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

/**
 * The contents of this file were heavily inspired by the source code of
 * Qt Creator, a Qt and C/C++ IDE by The Qt Company Ltd., 2016. That source code
 * is licensed under GNU GPL v.3 license with two permissive exceptions although
 * they don't apply to this derived work.
 */

#ifndef QUENTIER_DIALOGS_SHORTCUT_SETTINGS_SHORTCUT_BUTTON_H
#define QUENTIER_DIALOGS_SHORTCUT_SETTINGS_SHORTCUT_BUTTON_H

#include <quentier/utility/Macros.h>

#include <QKeySequence>
#include <QPushButton>

namespace quentier {

class ShortcutButton: public QPushButton
{
    Q_OBJECT
public:
    ShortcutButton(QWidget * parent = nullptr);

    virtual QSize sizeHint() const override;

Q_SIGNALS:
    void keySequenceChanged(const QKeySequence & sequence);

protected:
    virtual bool eventFilter(QObject * pWatched, QEvent * pEvent) override;

private:
    void updateText();

private Q_SLOTS:
    void handleToggleChange(bool toggleChange);

private:
    QString     m_uncheckedText;
    QString     m_checkedText;
    mutable int m_preferredWidth = -1;
    int         m_key[4] = {0, 0, 0, 0};
    int         m_keyNum = -1;
};

} // namespace quentier

#endif // QUENTIER_DIALOGS_SHORTCUT_SETTINGS_SHORTCUT_BUTTON_H
