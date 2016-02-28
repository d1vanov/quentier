#ifndef __LIB_QUTE_NOTE__NOTE_EDITOR__SPELL_CHECKER_H
#define __LIB_QUTE_NOTE__NOTE_EDITOR__SPELL_CHECKER_H

#include <qute_note/utility/Qt4Helper.h>
#include <QObject>
#include <QStringList>
#include <QVector>
#include <QPair>
#include <QSharedPointer>
#include <QUuid>
#include <QHash>

QT_FORWARD_DECLARE_CLASS(Hunspell)

namespace qute_note {

QT_FORWARD_DECLARE_CLASS(FileIOThreadWorker)

class SpellChecker: public QObject
{
    Q_OBJECT
public:
    SpellChecker(FileIOThreadWorker * pFileIOThreadWorker, QObject * parent = Q_NULLPTR,
                 const QString & userDictionaryPath = QString());

    // The second bool in the pair indicates whether the dictionary is enabled or disabled
    QVector<QPair<QString,bool> > listAvailableDictionaries() const;

    void enableDictionary(const QString & language);
    void disableDictionary(const QString & language);

    bool checkSpell(const QString & word) const;
    QStringList spellCorrectionSuggestions(const QString & misSpelledWord) const;
    void addToUserWordlist(const QString & word);
    void removeFromUserWordList(const QString & word);
    void ignoreWord(const QString & word);
    void removeWord(const QString & word);

Q_SIGNALS:
    void ready();

// private signals
    void readFile(QString absoluteFilePath, QUuid requestId);
    void writeFile(QString absoluteFilePath, QByteArray data, QUuid requestId, bool append);

private:
    void scanSystemDictionaries();
    void addSystemDictionary(const QString & path, const QString & name);

    void initializeUserDictionary(const QString & userDictionaryPath);
    bool checkUserDictionaryPath(const QString & userDictionaryPath) const;

    void checkUserDictionaryDataPendingWriting();

    void onAppendUserDictionaryPartDone(bool success, QString errorDescription);
    void onUpdateUserDictionaryDone(bool success, QString errorDescription);

private Q_SLOTS:
    void onReadFileRequestProcessed(bool success, QString errorDescription, QByteArray data, QUuid requestId);
    void onWriteFileRequestProcessed(bool success, QString errorDescription, QUuid requestId);

private:
    class Dictionary
    {
    public:
        Dictionary();

        bool isEmpty() const;

    public:
        QSharedPointer<Hunspell>    m_pHunspell;
        QString                     m_dictionaryPath;
        bool                        m_enabled;
    };

private:
    FileIOThreadWorker *        m_pFileIOThreadWorker;

    // Hashed by the language code
    QHash<QString, Dictionary>  m_systemDictionaries;

    QUuid                       m_readUserDictionaryRequestId;
    QString                     m_userDictionaryPath;
    QStringList                 m_userDictionary;
    bool                        m_userDictionaryReady;

    QStringList                 m_userDictionaryPartPendingWriting;
    QUuid                       m_appendUserDictionaryPartToFileRequestId;

    QUuid                       m_updateUserDictionaryFileRequestId;
};

} // namespace qute_note

#endif // __LIB_QUTE_NOTE__NOTE_EDITOR__SPELL_CHECKER_H
