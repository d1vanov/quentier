#ifndef __LIB_QUTE_NOTE__NOTE_EDITOR__DECRYPTED_TEXT_CACHE_H
#define __LIB_QUTE_NOTE__NOTE_EDITOR__DECRYPTED_TEXT_CACHE_H

#include <QSharedPointer>
#include <QHash>
#include <QString>

namespace qute_note {

/**
 * @brief DecryptedTextCachePtr defines the type for the cache of decrypted text entries.
 *
 * This cache can be used (and is in fact intended to be used) in two ways:
 *
 * First, it can cache text which has already been properly decrypted with the provided passphrase
 * but which is not expected to be "decrypted for session" i.e. when the note with this encrypted text
 * is closed and opened again, it should appear encrypted again. Such cached text entries can use passphrase
 * as the key to avoid calling the expensive decryption routines each time user decrypts them
 * with the passphrase.
 *
 * The second way is for the text which has been decrypted once and should stay decrypted
 * through the whole working session; such text can use encrypted text as the key for decrypted text.
 * So when the note with encrypted text which has already been decrypted before is loaded into the note editor,
 * this cache can be used to figure out which of encrypted text entries should become decrypted
 * right away, without prompting user for the passphrase again.
 *
 * The bool in the value pair controls exactly the way the cache entry is used by:
 * when it's true, it means the cache entry is for the text which should be "remembered through the whole session". \
 * When it's false, it's the "internal" cache entry purely for speeding up the decryption
 * of the text which has actually already been decrypted before.
 */
typedef QSharedPointer<QHash<QString, QPair<QString, bool> > > DecryptedTextCachePtr;

} // namespace qute_note

#endif // __LIB_QUTE_NOTE__NOTE_EDITOR__DECRYPTED_TEXT_CACHE_H
