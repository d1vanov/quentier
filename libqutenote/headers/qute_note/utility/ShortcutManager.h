#ifndef __LIB_QUTE_NOTE__UTILITY__SHORTCUT_MANAGER_H
#define __LIB_QUTE_NOTE__UTILITY__SHORTCUT_MANAGER_H

#include <qute_note/utility/Linkage.h>
#include <qute_note/utility/Qt4Helper.h>
#include <QObject>
#include <QKeySequence>

namespace qute_note {

QT_FORWARD_DECLARE_CLASS(ShortcutManagerPrivate)

class QUTE_NOTE_EXPORT ShortcutManager: public QObject
{
    Q_OBJECT
public:
    explicit ShortcutManager(QObject * parent = Q_NULLPTR);

    enum QuteNoteShortcutKey
    {
        NewNote = 5000,
        NewTag,
        NewNotebook,
        NewSavedSearch,
        AddAttachment,
        SaveAttachment,
        OpenAttachment,
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
        ShowTrash,
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

} // namespace qute_note

#endif // __LIB_QUTE_NOTE__UTILITY__SHORTCUT_MANAGER_H

