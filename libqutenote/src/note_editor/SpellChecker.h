#ifndef __LIB_QUTE_NOTE__NOTE_EDITOR__SPELL_CHECKER_H
#define __LIB_QUTE_NOTE__NOTE_EDITOR__SPELL_CHECKER_H

#include <qute_note/utility/Qt4Helper.h>
#include <QObject>
#include <QStringList>
#include <QVector>
#include <QSharedPointer>
#include <QUuid>

QT_FORWARD_DECLARE_CLASS(Hunspell)

namespace qute_note {

QT_FORWARD_DECLARE_CLASS(FileIOThreadWorker)

class SpellChecker: public QObject
{
    Q_OBJECT
public:
    SpellChecker(FileIOThreadWorker * pFileIOThreadWorker, QObject * parent = Q_NULLPTR, const QString & userDictionaryPath = QString());

    bool isValid() const;

    bool checkSpell(const QString & word) const;
    QStringList spellCorrectionSuggestions(const QString & misSpelledWord) const;
    void addToUserWordlist(const QString & word);
    void ignoreWord(const QString & word);

Q_SIGNALS:
    void ready();

private:
    void scanSystemDictionaries();
    void initializeUserDictionary(const QString & userDictionaryPath);
    bool checkUserDictionaryPath(const QString & userDictionaryPath) const;

    void onUserDictionaryFileRead(bool success, QString errorDescription, QByteArray data, QUuid requestId);
    void onUserAffixFileRead(bool success, QString & errorDescription, QByteArray data, QUuid requestId);

    void onSystemDictionaryFileRead(bool success, QString & errorDescription, QByteArray data, QUuid requestId,
                                    const qint64 dictionaryIndex);
    void onSystemAffixFileRead(bool success, QString & errorDescription, QByteArray data, QUuid requestId,
                               const qint64 dictionaryIndex);

    void checkAndNotifyDictionariesReadiness();

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

        QUuid                       m_readDictionaryFileRequestId;
        QUuid                       m_readAffixFileRequestId;

        QByteArray                  m_dictionaryFileData;
        QByteArray                  m_affixFileData;

        bool                        m_dictionaryFileRead;
        bool                        m_affixFileRead;
    };

    class CompareByReadDictionaryFileRequestId
    {
    public:
        CompareByReadDictionaryFileRequestId(const QUuid & requestId) : m_requestId(requestId) {}
        bool operator()(const Dictionary & dictionary) const { return m_requestId == dictionary.m_readDictionaryFileRequestId; }

    private:
        QUuid   m_requestId;
    };

    class CompareByReadAffixFileRequestId
    {
    public:
        CompareByReadAffixFileRequestId(const QUuid & requestId) : m_requestId(requestId) {}
        bool operator()(const Dictionary & dictionary) const { return m_requestId == dictionary.m_readAffixFileRequestId; }

    private:
        QUuid   m_requestId;
    };

private:
    FileIOThreadWorker *    m_pFileIOThreadWorker;
    QVector<Dictionary>     m_systemDictionaries;
    Dictionary              m_userDictionary;
};

} // namespace qute_note

#endif // __LIB_QUTE_NOTE__NOTE_EDITOR__SPELL_CHECKER_H
