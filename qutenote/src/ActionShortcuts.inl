// File menu
PROCESS_ACTION_SHORTCUT(NewNote, ShortcutManager::NewNote);
PROCESS_ACTION_SHORTCUT(NewNotebook, ShortcutManager::NewNotebook);
PROCESS_ACTION_SHORTCUT(NewTag, ShortcutManager::NewTag);
PROCESS_ACTION_SHORTCUT(NewSavedSearch, ShortcutManager::NewSavedSearch);
PROCESS_ACTION_SHORTCUT(Print, QKeySequence::Print);
PROCESS_ACTION_SHORTCUT(Quit, QKeySequence::Quit);

// Edit menu
PROCESS_ACTION_SHORTCUT(Undo, QKeySequence::Undo);
PROCESS_ACTION_SHORTCUT(Redo, QKeySequence::Redo);
PROCESS_ACTION_SHORTCUT(Cut, QKeySequence::Cut);
PROCESS_ACTION_SHORTCUT(Copy, QKeySequence::Copy);
PROCESS_ACTION_SHORTCUT(Paste, QKeySequence::Paste);
PROCESS_ACTION_SHORTCUT(PasteUnformatted, ShortcutManager::PasteUnformatted);
PROCESS_ACTION_SHORTCUT(SelectAll, QKeySequence::SelectAll);
PROCESS_ACTION_SHORTCUT(Delete, QKeySequence::Delete);

// Find and replace menu
PROCESS_ACTION_SHORTCUT(FindNote, ShortcutManager::NoteSearch);
PROCESS_ACTION_SHORTCUT(FindInsideNote, QKeySequence::Find);
PROCESS_ACTION_SHORTCUT(FindNext, QKeySequence::FindNext);
PROCESS_ACTION_SHORTCUT(FindPrevious, QKeySequence::FindPrevious);
PROCESS_ACTION_SHORTCUT(ReplaceInNote, QKeySequence::Replace);
PROCESS_ACTION_SHORTCUT(SpellCheck, ShortcutManager::SpellCheck);

// View menu
PROCESS_NON_STANDARD_ACTION_SHORTCUT(Notes, "View");
PROCESS_NON_STANDARD_ACTION_SHORTCUT(Notebooks, "View");
PROCESS_NON_STANDARD_ACTION_SHORTCUT(Tags, "View");
PROCESS_NON_STANDARD_ACTION_SHORTCUT(SavedSearches, "View");
PROCESS_NON_STANDARD_ACTION_SHORTCUT(ShowSidePanel, "View");
PROCESS_NON_STANDARD_ACTION_SHORTCUT(ShowFavorites, "View");
PROCESS_ACTION_SHORTCUT(ShowNotes, ShortcutManager::ShowNotes, "View");
PROCESS_ACTION_SHORTCUT(ShowNotebooks, ShortcutManager::ShowNotebooks, "View");
PROCESS_ACTION_SHORTCUT(ShowTags, ShortcutManager::ShowTags, "View");
PROCESS_ACTION_SHORTCUT(ShowSavedSearches, ShortcutManager::ShowSavedSearches, "View");
PROCESS_ACTION_SHORTCUT(ShowTrash, ShortcutManager::ShowTrash, "View");
PROCESS_NON_STANDARD_ACTION_SHORTCUT(ShowNotesList, "View");
PROCESS_NON_STANDARD_ACTION_SHORTCUT(ShowNoteEditor, "View");
PROCESS_NON_STANDARD_ACTION_SHORTCUT(ShowToolbar, "View");
PROCESS_NON_STANDARD_ACTION_SHORTCUT(ShowStatusBar, "View");

// Format menu
PROCESS_ACTION_SHORTCUT(FontBold, QKeySequence::Bold);
PROCESS_ACTION_SHORTCUT(FontUnderlined, QKeySequence::Underline);
PROCESS_ACTION_SHORTCUT(FontItalic, QKeySequence::Italic);
PROCESS_ACTION_SHORTCUT(FontStrikethrough, ShortcutManager::Strikethrough);
PROCESS_ACTION_SHORTCUT(FontHighlight, ShortcutManager::Highlight);
PROCESS_ACTION_SHORTCUT(FontUpperIndex, ShortcutManager::UpperIndex);
PROCESS_ACTION_SHORTCUT(FontLowerIndex, ShortcutManager::LowerIndex);
PROCESS_ACTION_SHORTCUT(IncreaseFontSize, ShortcutManager::IncreaseFontSize);
PROCESS_ACTION_SHORTCUT(DecreaseFontSize, ShortcutManager::DecreaseFontSize);
PROCESS_ACTION_SHORTCUT(InsertHorizontalLine, ShortcutManager::InsertHorizontalLine);

PROCESS_ACTION_SHORTCUT(InsertTable, ShortcutManager::InsertTable);
PROCESS_ACTION_SHORTCUT(InsertRow, ShortcutManager::InsertRow);
PROCESS_ACTION_SHORTCUT(InsertColumn, ShortcutManager::InsertColumn);
PROCESS_ACTION_SHORTCUT(RemoveRow, ShortcutManager::RemoveRow);
PROCESS_ACTION_SHORTCUT(RemoveColumn, ShortcutManager::RemoveColumn);

PROCESS_ACTION_SHORTCUT(SaveImage, ShortcutManager::SaveImage);
PROCESS_ACTION_SHORTCUT(RotateClockwise, ShortcutManager::ImageRotateClockwise);
PROCESS_ACTION_SHORTCUT(RotateCounterClockwise, ShortcutManager::ImageRotateCounterClockwise);

PROCESS_ACTION_SHORTCUT(AlignLeft, ShortcutManager::AlignLeft);
PROCESS_ACTION_SHORTCUT(AlignCenter, ShortcutManager::AlignCenter);
PROCESS_ACTION_SHORTCUT(AlignRight, ShortcutManager::AlignRight);

PROCESS_ACTION_SHORTCUT(InsertBulletedList, ShortcutManager::InsertBulletedList);
PROCESS_ACTION_SHORTCUT(InsertNumberedList, ShortcutManager::InsertNumberedList);

PROCESS_ACTION_SHORTCUT(IncreaseIndentation, ShortcutManager::IncreaseIndentation);
PROCESS_ACTION_SHORTCUT(DecreaseIndentation, ShortcutManager::DecreaseIndentation);

// Service menu
PROCESS_ACTION_SHORTCUT(Synchronize, ShortcutManager::Synchronize);
PROCESS_ACTION_SHORTCUT(AccountInfo, ShortcutManager::AccountInfo);
PROCESS_ACTION_SHORTCUT(LocalStorageStatus, ShortcutManager::LocalStorageStatus);
PROCESS_ACTION_SHORTCUT(ImportFolders, ShortcutManager::ImportFolders);
PROCESS_ACTION_SHORTCUT(Preferences, QKeySequence::Preferences);

// Help menu
PROCESS_ACTION_SHORTCUT(ReleaseNotes, ShortcutManager::ReleaseNotes);
PROCESS_ACTION_SHORTCUT(ViewLogs, ShortcutManager::ViewLogs);
PROCESS_ACTION_SHORTCUT(About, ShortcutManager::About);
