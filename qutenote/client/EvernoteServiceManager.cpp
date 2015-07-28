#include "EvernoteServiceManager.h"
#include "../src/MainWindow.h"
#include "../src/EvernoteOAuthBrowser.h"
#include <Simplecrypt.h>
#include <QEverCloudOAuth.h>
#include <vector>
#include <string>
#include <QtCore>
#include <QFile>
#include <QMessageBox>

class EvernoteServiceManager::EvernoteDataHolder
{
public:
    EvernoteDataHolder() :
        m_userStoreClientPtr(nullptr),
        m_noteStoreClientPtr(nullptr),
        m_notebookPtr(nullptr),
        m_trashNotebookPtr(nullptr),
        m_favouriteTagPtr(nullptr),
        m_noteStoreUrl("")
    {}

    qevercloud::UserStore *& userStorePtr() {
        return m_userStoreClientPtr;
    }

    qevercloud::NoteStore *& noteStorePtr()
    {
        return m_noteStoreClientPtr;
    }

    qevercloud::Notebook *& notebookPtr()
    {
        return m_notebookPtr;
    }

    qevercloud::Notebook *& trashNotebookPtr()
    {
        return m_trashNotebookPtr;
    }

    qevercloud::Tag *& favouriteTagPtr()
    {
        return m_favouriteTagPtr;
    }

    QString & noteStoreUrl()
    {
        return m_noteStoreUrl;
    }

private:
    qevercloud::UserStore * m_userStoreClientPtr;
    qevercloud::NoteStore * m_noteStoreClientPtr;
    qevercloud::Notebook * m_notebookPtr;
    qevercloud::Notebook * m_trashNotebookPtr;
    qevercloud::Tag * m_favouriteTagPtr;
    QString m_noteStoreUrl;
};

void EvernoteServiceManager::authenticate()
{
    // TODO: implement using QEverCloud
}

void EvernoteServiceManager::connect()
{
    /*
    QString errorMessage;
    if (!CheckAuthenticationState(errorMessage)) {
        emit statusTextUpdate(QString(tr("Unable to connect to Evernote - authentication failed: ")) +
                              errorMessage, 0);
        return;
    }

    // std::string username = m_credentials.GetUsername().toStdString();
    // std::string password = m_credentials.GetPassword().toStdString();
    // std::string consumerKey = m_credentials.GetConsumerKey().toStdString();
    // std::string consumerSecret = m_credentials.GetConsumerSecret().toStdString();
    QString authToken = m_credentials.GetOAuthKey();
    // std::string authTokenSecret = m_credentials.GetOAuthSecret().toStdString();

    QString hostname = GetHostName();

    qevercloud::UserStore * pUserStore = m_pEvernoteDataHolder->userStorePtr();
    if (pUserStore == nullptr) {
        delete pUserStore;
        pUserStore = nullptr;
    }

    pUserStore = new qevercloud::UserStore(hostname, authToken, this);

    QString noteStoreUrl = pUserStore->getNoteStoreUrl();

    if (m_pEvernoteDataHolder == nullptr) {
        emit statusTextUpdate(QString(tr("Unable to connect to Evernote: data holder ptr is null! ")) +
                        QString(tr("Please contact application developer.")), 0);
        return;
    }

    m_pEvernoteDataHolder->noteStoreUrl() = noteStoreUrl;
    QString path = noteStoreUrl;
    path.replace("https://", "");
    path.replace(hostname, "");

    qevercloud::NoteStore * pNoteStore = m_pEvernoteDataHolder->noteStorePtr();
    if (pNoteStore != nullptr) {
        delete pNoteStore;
        pNoteStore = nullptr;
    }

    pNoteStore = new qevercloud::NoteStore(noteStoreUrl, authToken, this);

    SetDefaultNotebook();
    SetTrashNotebook();
    SetFavouriteTag();
    SetConnectionState(ECS_CONNECTED);
    // TODO: set up timer to refresh connection, synchronize
    */
}

void EvernoteServiceManager::disconnect()
{
    if (m_pEvernoteDataHolder == nullptr) {
        return;
    }

    // TODO: figure out what to do here

    SetConnectionState(ECS_DISCONNECTED);
}

void EvernoteServiceManager::setRefreshTime(const double refreshTime)
{
    m_refreshTime = refreshTime;
}

EvernoteServiceManager::EvernoteServiceManager() :
    m_pEvernoteDataHolder(new EvernoteDataHolder),
    m_credentials(this),
    m_authorizationState(EAS_UNAUTHORIZED_NEVER_ATTEMPTED),
    m_connectionState(ECS_DISCONNECTED),
    m_evernoteHostName("https://sandbox.evernote.com"),
    m_refreshTime(0.0)
{}

bool EvernoteServiceManager::CheckConnectionState() const
{
    if (m_connectionState == ECS_CONNECTED) {
        return true;
    }
    else {
        return false;
    }
}

void EvernoteServiceManager::SetConnectionState(const EvernoteServiceManager::EConnectionState connectionState)
{
    m_connectionState = connectionState;
}

void EvernoteServiceManager::SetDefaultNotebook()
{
    if (m_pEvernoteDataHolder == nullptr) {
        emit statusTextUpdate(QString(tr("Unable to set default notebook: EvernoteDataHolder is null. ")) +
                        QString(tr("Please contact application developer.")), 0);
        return;
    }

    qevercloud::NoteStore * pNoteStore = m_pEvernoteDataHolder->noteStorePtr();
    if (pNoteStore == nullptr) {
        emit statusTextUpdate(QString(tr("Unable to set default notebook: NoteStoreClient is null. ")) +
                              QString(tr("Please contact application developer.")), 0);
        return;
    }

    QList<qevercloud::Notebook> notebooks = pNoteStore->listNotebooks();
    qevercloud::Notebook defaultNotebook = pNoteStore->getDefaultNotebook();

    qevercloud::Notebook *& pNotebook = m_pEvernoteDataHolder->notebookPtr();
    int numNotebooks = notebooks.size();
    for(int i = 0; i < numNotebooks; ++i)
    {
        const qevercloud::Notebook & notebook = notebooks.at(i);
        if (notebook.guid.ref() == defaultNotebook.guid.ref())
        {   
            if (!pNotebook) {
                pNotebook = new qevercloud::Notebook(notebook);
            }
            else {
                *pNotebook = notebook;
            }
            return;
        }
    }

    // Default notebook not found, creating one
    defaultNotebook = pNoteStore->createNotebook(defaultNotebook);
    defaultNotebook = pNoteStore->getNotebook(defaultNotebook.guid);

    if (!pNotebook) {
        pNotebook = new qevercloud::Notebook(defaultNotebook);
    }
    else {
        *pNotebook = defaultNotebook;
    }
}

void EvernoteServiceManager::SetTrashNotebook()
{
    if (m_pEvernoteDataHolder == nullptr) {
        emit statusTextUpdate(QString(tr("Unable to set default notebook: EvernoteDataHolder is null. ")) +
                        QString(tr("Please contact application developer.")), 0);
        return;
    }

    qevercloud::NoteStore *& pNoteStore = m_pEvernoteDataHolder->noteStorePtr();
    if (!pNoteStore) {
        emit statusTextUpdate(QString(tr("Unable to set default notebook: NoteStoreClient is null. ")) +
                        QString(tr("Please contact application developer.")), 0);
        return;
    }

    qevercloud::Notebook *& pTrashNotebook = m_pEvernoteDataHolder->trashNotebookPtr();
    if (!pTrashNotebook)
    {
        pTrashNotebook = new qevercloud::Notebook;
        pTrashNotebook->name = QString(tr("Trash notebook"));
        *pTrashNotebook = pNoteStore->createNotebook(*pTrashNotebook);
    }
    else
    {
        QList<qevercloud::Notebook> notebooks = pNoteStore->listNotebooks();
        int numNotebooks = notebooks.size();
        for(int i = 0; i < numNotebooks; ++i)
        {
            const qevercloud::Notebook & notebook = notebooks.at(i);
            if (notebook.guid.ref() == pTrashNotebook->guid.ref())
            {
                *pTrashNotebook = notebook;
                return;
            }
        }

        // Trash notebook not found, creating one
        *pTrashNotebook = qevercloud::Notebook();
        pTrashNotebook->name = QString(tr("Trash notebook"));
        *pTrashNotebook = pNoteStore->createNotebook(*pTrashNotebook);
    }
}

void EvernoteServiceManager::SetFavouriteTag()
{
    if (m_pEvernoteDataHolder == nullptr) {
        emit statusTextUpdate(QString(tr("Unable to set favourite tag: EvernoteDataHolder is null. ")) +
                        QString(tr("Please contact application developer.")), 0);
        return;
    }

    qevercloud::NoteStore *& pNoteStore = m_pEvernoteDataHolder->noteStorePtr();
    if (!pNoteStore) {
        emit statusTextUpdate(QString(tr("Unable to set favourite tag: NoteStoreClient is null. ")) +
                        QString(tr("Please contact application developer.")), 0);
        return;
    }

    qevercloud::Tag *& pFavouriteTag = m_pEvernoteDataHolder->favouriteTagPtr();
    if (!pFavouriteTag)
    {
        pFavouriteTag = new qevercloud::Tag;
        pFavouriteTag->name = QString(tr("Favourite tag"));
        *pFavouriteTag = pNoteStore->createTag(*pFavouriteTag);
        return;
    }
    else
    {
        QList<qevercloud::Tag> tags = pNoteStore->listTags();
        int numTags = tags.size();
        for(int i = 0; i < numTags; ++i)
        {
            const qevercloud::Tag & tag = tags.at(i);
            if (tag.guid.ref() == pFavouriteTag->guid.ref())
            {
                *pFavouriteTag = tag;
                return;
            }
        }

        // Favourite tag not found, creating one
        *pFavouriteTag = qevercloud::Tag();
        pFavouriteTag->name = QString(tr("Favourite tag"));
        *pFavouriteTag = pNoteStore->createTag(*pFavouriteTag);
    }
}

bool EvernoteServiceManager::setCredentials(const CredentialsModel & credentials,
                                            QString & errorMessage)
{
    if (credentials.Empty(errorMessage)) {
        return false;
    }

    m_credentials = credentials;
    return true;
}

bool EvernoteServiceManager::CheckAuthenticationState(QString & errorMessage) const
{
    switch(m_authorizationState)
    {
    case EAS_AUTHORIZED:
        return true;
    case EAS_UNAUTHORIZED_NEVER_ATTEMPTED:
        errorMessage = tr("Not authorized yet");
        return false;
    case EAS_UNAUTHORIZED_CREDENTIALS_REJECTED:
        errorMessage = tr("Last authorization attempt failed: credentials rejected");
        return false;
    case EAS_UNAUTHORIZED_QUIT:
        errorMessage = tr("Not yet authorized after last quit");
        return false;
    case EAS_UNAUTHORIZED_INTERNAL_ERROR:
        errorMessage = tr("Not authorized due to internal error, please contact developers");
        return false;
    default:
        errorMessage = tr("Not authorized: unknown error, please contact developers");
        return false;
    }
}

const QString EvernoteServiceManager::GetHostName() const
{
    return m_evernoteHostName;
}

void EvernoteServiceManager::onOAuthSuccess(QString key, QString secret)
{
    m_credentials.SetOAuthKey(key);
    m_credentials.SetOAuthSecret(secret);

    SimpleCrypt crypto(CredentialsModel::RANDOM_CRYPTO_KEY);
    QString encryptedKey = crypto.encryptToString(key);
    QString encryptedSecret = crypto.encryptToString(secret);
    QFile tokensFile(":/enc_data/oautk.dat");
    if (!tokensFile.open(QIODevice::WriteOnly | QIODevice::Text)) {
        QMessageBox::information(0, tr("Error: cannot open file with OAuth tokens: "),
                                 tokensFile.errorString());
        m_authorizationState = EAS_UNAUTHORIZED_INTERNAL_ERROR;
        emit statusTextUpdate(tr("Got OAuth tokens but unable to save them encrypted in file"), 0);
        return;
    }

#if QT_VERSION >= 0x050101
    tokensFile.write(encryptedKey.toUtf8());
    tokensFile.write(encryptedSecret.toUtf8());
#else
    tokensFile.write(encryptedKey.toAscii());
    tokensFile.write(encryptedSecret.toAscii());
#endif
    tokensFile.close();

    m_authorizationState = EAS_AUTHORIZED;
    emit statusTextUpdate("Successfully authenticated to Evernote!", 2000);
}

void EvernoteServiceManager::onOAuthFailure(QString message)
{
    // TODO: think twice how should I deduce the state here
    m_authorizationState = EAS_UNAUTHORIZED_CREDENTIALS_REJECTED;
    emit statusTextUpdate("Unable to authenticate to Evernote: " + message, 0);
}

void EvernoteServiceManager::onRequestToShowAuthorizationPage(QUrl authUrl)
{
    emit showAuthWebPage(authUrl);
}

void EvernoteServiceManager::onConsumerKeyAndSecretSet(QString key, QString secret)
{
    m_credentials.SetConsumerKey(key);
    m_credentials.SetConsumerSecret(secret);

    if (m_credentials.GetUsername().isEmpty() ||
        m_credentials.GetPassword().isEmpty())
    {
        emit requestUsernameAndPassword();
    }
}

void EvernoteServiceManager::onUserNameAndPasswordSet(QString name, QString password)
{
    m_credentials.SetUsername(name);
    m_credentials.SetPassword(password);
}
