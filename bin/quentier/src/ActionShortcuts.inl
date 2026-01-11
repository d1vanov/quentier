// File menu
PROCESS_ACTION_SHORTCUT(NewNote, quentier::utility::ShortcutManager::NewNote, "File");
PROCESS_ACTION_SHORTCUT(NewNotebook, quentier::utility::ShortcutManager::NewNotebook, "File");
PROCESS_ACTION_SHORTCUT(NewTag, quentier::utility::ShortcutManager::NewTag, "File");
PROCESS_ACTION_SHORTCUT(NewSavedSearch, quentier::utility::ShortcutManager::NewSavedSearch, "File");
PROCESS_ACTION_SHORTCUT(Print, QKeySequence::Print, "File");
PROCESS_ACTION_SHORTCUT(Quit, QKeySequence::Quit, "File");

// Edit menu
PROCESS_ACTION_SHORTCUT(Undo, QKeySequence::Undo, "Edit");
PROCESS_ACTION_SHORTCUT(Redo, QKeySequence::Redo, "Edit");
PROCESS_ACTION_SHORTCUT(Cut, QKeySequence::Cut, "Edit");
PROCESS_ACTION_SHORTCUT(Copy, QKeySequence::Copy, "Edit");
PROCESS_ACTION_SHORTCUT(Paste, QKeySequence::Paste, "Edit");
PROCESS_ACTION_SHORTCUT(PasteUnformatted, quentier::utility::ShortcutManager::PasteUnformatted, "Edit");
PROCESS_ACTION_SHORTCUT(SelectAll, QKeySequence::SelectAll, "Edit");
PROCESS_ACTION_SHORTCUT(Delete, QKeySequence::Delete, "Edit");
PROCESS_ACTION_SHORTCUT(SaveNote, QKeySequence::Save, "Edit");

// Find and replace menu
PROCESS_ACTION_SHORTCUT(FindNote, quentier::utility::ShortcutManager::NoteSearch, "Edit");
PROCESS_ACTION_SHORTCUT(FindInsideNote, QKeySequence::Find, "Edit");
PROCESS_ACTION_SHORTCUT(FindNext, QKeySequence::FindNext, "Edit");
PROCESS_ACTION_SHORTCUT(FindPrevious, QKeySequence::FindPrevious, "Edit");
PROCESS_ACTION_SHORTCUT(ReplaceInNote, QKeySequence::Replace, "Edit");
PROCESS_ACTION_SHORTCUT(SpellCheck, quentier::utility::ShortcutManager::SpellCheck, "Edit");

// View menu
PROCESS_NON_STANDARD_ACTION_SHORTCUT(Notes, "View");
PROCESS_NON_STANDARD_ACTION_SHORTCUT(Notebooks, "View");
PROCESS_NON_STANDARD_ACTION_SHORTCUT(Tags, "View");
PROCESS_NON_STANDARD_ACTION_SHORTCUT(SavedSearches, "View");
PROCESS_NON_STANDARD_ACTION_SHORTCUT(ShowSidePanel, "View");
PROCESS_NON_STANDARD_ACTION_SHORTCUT(ShowFavorites, "View");
PROCESS_ACTION_SHORTCUT(ShowNotebooks, quentier::utility::ShortcutManager::ShowNotebooks, "View");
PROCESS_ACTION_SHORTCUT(ShowTags, quentier::utility::ShortcutManager::ShowTags, "View");
PROCESS_ACTION_SHORTCUT(ShowSavedSearches, quentier::utility::ShortcutManager::ShowSavedSearches, "View");
PROCESS_ACTION_SHORTCUT(ShowDeletedNotes, quentier::utility::ShortcutManager::ShowDeletedNotes, "View");
PROCESS_NON_STANDARD_ACTION_SHORTCUT(ShowNotesList, "View");
PROCESS_NON_STANDARD_ACTION_SHORTCUT(ShowNoteEditor, "View");
PROCESS_NON_STANDARD_ACTION_SHORTCUT(ShowToolbar, "View");
PROCESS_NON_STANDARD_ACTION_SHORTCUT(ShowStatusBar, "View");

// Format menu
PROCESS_ACTION_SHORTCUT(FontBold, QKeySequence::Bold, "Format");
PROCESS_ACTION_SHORTCUT(FontUnderlined, QKeySequence::Underline, "Format");
PROCESS_ACTION_SHORTCUT(FontItalic, QKeySequence::Italic, "Format");
PROCESS_ACTION_SHORTCUT(FontStrikethrough, quentier::utility::ShortcutManager::Strikethrough, "Format");
PROCESS_ACTION_SHORTCUT(FontHighlight, quentier::utility::ShortcutManager::Highlight, "Format");
PROCESS_ACTION_SHORTCUT(FontUpperIndex, quentier::utility::ShortcutManager::UpperIndex, "Format");
PROCESS_ACTION_SHORTCUT(FontLowerIndex, quentier::utility::ShortcutManager::LowerIndex, "Format");
PROCESS_ACTION_SHORTCUT(IncreaseFontSize, quentier::utility::ShortcutManager::IncreaseFontSize, "Format");
PROCESS_ACTION_SHORTCUT(DecreaseFontSize, quentier::utility::ShortcutManager::DecreaseFontSize, "Format");
PROCESS_ACTION_SHORTCUT(InsertHorizontalLine, quentier::utility::ShortcutManager::InsertHorizontalLine, "Format");

PROCESS_ACTION_SHORTCUT(InsertTable, quentier::utility::ShortcutManager::InsertTable, "Format");
PROCESS_ACTION_SHORTCUT(InsertRow, quentier::utility::ShortcutManager::InsertRow, "Format");
PROCESS_ACTION_SHORTCUT(InsertColumn, quentier::utility::ShortcutManager::InsertColumn, "Format");
PROCESS_ACTION_SHORTCUT(RemoveRow, quentier::utility::ShortcutManager::RemoveRow, "Format");
PROCESS_ACTION_SHORTCUT(RemoveColumn, quentier::utility::ShortcutManager::RemoveColumn, "Format");

PROCESS_ACTION_SHORTCUT(AlignLeft, quentier::utility::ShortcutManager::AlignLeft, "Format");
PROCESS_ACTION_SHORTCUT(AlignCenter, quentier::utility::ShortcutManager::AlignCenter, "Format");
PROCESS_ACTION_SHORTCUT(AlignRight, quentier::utility::ShortcutManager::AlignRight, "Format");
PROCESS_ACTION_SHORTCUT(AlignFull, quentier::utility::ShortcutManager::AlignFull, "Format");

PROCESS_ACTION_SHORTCUT(InsertBulletedList, quentier::utility::ShortcutManager::InsertBulletedList, "Format");
PROCESS_ACTION_SHORTCUT(InsertNumberedList, quentier::utility::ShortcutManager::InsertNumberedList, "Format");

PROCESS_ACTION_SHORTCUT(InsertToDo, quentier::utility::ShortcutManager::InsertToDoTag, "Format")

PROCESS_ACTION_SHORTCUT(IncreaseIndentation, quentier::utility::ShortcutManager::IncreaseIndentation, "Format");
PROCESS_ACTION_SHORTCUT(DecreaseIndentation, quentier::utility::ShortcutManager::DecreaseIndentation, "Format");

PROCESS_ACTION_SHORTCUT(EditHyperlink, quentier::utility::ShortcutManager::EditHyperlink, "Format");
PROCESS_ACTION_SHORTCUT(CopyHyperlink, quentier::utility::ShortcutManager::CopyHyperlink, "Format");
PROCESS_ACTION_SHORTCUT(RemoveHyperlink, quentier::utility::ShortcutManager::RemoveHyperlink, "Format");

// Service menu
PROCESS_ACTION_SHORTCUT(Synchronize, quentier::utility::ShortcutManager::Synchronize, "Service");
PROCESS_ACTION_SHORTCUT(AccountInfo, quentier::utility::ShortcutManager::AccountInfo, "Service");
PROCESS_ACTION_SHORTCUT(LocalStorageStatus, quentier::utility::ShortcutManager::LocalStorageStatus, "Service");
PROCESS_ACTION_SHORTCUT(ImportFolders, quentier::utility::ShortcutManager::ImportFolders, "Service");

// General menu
PROCESS_ACTION_SHORTCUT(Preferences, QKeySequence::Preferences, "General");

// Help menu
PROCESS_ACTION_SHORTCUT(ReleaseNotes, quentier::utility::ShortcutManager::ReleaseNotes, "Help");
PROCESS_ACTION_SHORTCUT(ViewLogs, quentier::utility::ShortcutManager::ViewLogs, "Help");
PROCESS_ACTION_SHORTCUT(About, quentier::utility::ShortcutManager::About, "Help");
