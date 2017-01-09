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

#ifndef LIB_QUENTIER_UTILITY_SHORTCUT_MANAGER_H
#define LIB_QUENTIER_UTILITY_SHORTCUT_MANAGER_H

#include <quentier/utility/Linkage.h>
#include <quentier/utility/Macros.h>
#include <QObject>
#include <QKeySequence>

namespace quentier {

QT_FORWARD_DECLARE_CLASS(ShortcutManagerPrivate)

class QUENTIER_EXPORT ShortcutManager: public QObject
{
    Q_OBJECT
public:
    explicit ShortcutManager(QObject * parent = Q_NULLPTR);

    enum QuentierShortcutKey
    {
        NewNote = 5000,
        NewTag,
        NewNotebook,
        NewSavedSearch,
        AddAttachment,
        SaveAttachment,
        OpenAttachment,
        CopyAttachment,
        CutAttachment,
        RemoveAttachment,
        RenameAttachment,
        AddAccount,
        ExitAccount,
        SwitchAccount,
        AccountInfo,
        NoteSearch,
        NewNoteSearch,
        ShowNotes,
        ShowNotebooks,
        ShowTags,
        ShowSavedSearches,
        ShowDeletedNotes,
        ShowStatusBar,
        ShowToolBar,
        PasteUnformatted,
        Font,
        UpperIndex,
        LowerIndex,
        AlignLeft,
        AlignCenter,
        AlignRight,
        IncreaseIndentation,
        DecreaseIndentation,
        IncreaseFontSize,
        DecreaseFontSize,
        InsertNumberedList,
        InsertBulletedList,
        Strikethrough,
        Highlight,
        InsertTable,
        InsertRow,
        InsertColumn,
        RemoveRow,
        RemoveColumn,
        InsertHorizontalLine,
        InsertToDoTag,
        EditHyperlink,
        CopyHyperlink,
        RemoveHyperlink,
        Encrypt,
        Decrypt,
        DecryptPermanently,
        BackupLocalStorage,
        RestoreLocalStorage,
        UpgradeLocalStorage,
        LocalStorageStatus,
        SpellCheck,
        SpellCheckIgnoreWord,
        SpellCheckAddWordToUserDictionary,
        SaveImage,
        AnnotateImage,
        ImageRotateClockwise,
        ImageRotateCounterClockwise,
        Synchronize,
        FullSync,
        ImportFolders,
        Preferences,
        ReleaseNotes,
        ViewLogs,
        About,
        UnknownKey = 100000
    };

    QKeySequence shortcut(const int key, const QString & context = QString()) const;
    QKeySequence shortcut(const QString & nonStandardKey, const QString & context = QString()) const;

Q_SIGNALS:
    void shortcutChanged(int key, QKeySequence shortcut, QString context);
    void nonStandardShortcutChanged(QString nonStandardKey, QKeySequence shortcut, QString context);

public Q_SLOTS:
    void setUserShortcut(int key, QKeySequence shortcut, QString context = QString());
    void setNonStandardUserShortcut(QString nonStandardKey, QKeySequence shortcut, QString context = QString());

    void setDefaultShortcut(int key, QKeySequence shortcut, QString context = QString());
    void setNonStandardDefaultShortcut(QString nonStandardKey, QKeySequence shortcut, QString context = QString());

private:
    ShortcutManagerPrivate * const d_ptr;
    Q_DECLARE_PRIVATE(ShortcutManager)
};

} // namespace quentier

#endif // LIB_QUENTIER_UTILITY_SHORTCUT_MANAGER_H

