#include "EvernoteServiceManager.h"
#include "EvernoteServiceOAuthHandler.h"
#include "../gui/MainWindow.h"
#include "../gui/EvernoteOAuthBrowser.h"
#include "../tools/Singleton.h"
#include "../../SimpleCrypt/src/simplecrypt.h"
#include "../thrift/transport/THttpClient.h"
#ifdef HAVE_NETINET_IN_H
#include <netinet/in.h>
#else
#include <Winsock2.h>
#endif
#include "../thrift/transport/TSSLSocket.h"
#include "../thrift/transport/TBufferTransports.h"
#include "../thrift/protocol/TBinaryProtocol.h"
#include "../evernote_sdk/src/UserStore.h"
#include "../evernote_sdk/src/NoteStore.h"
#include <boost/shared_ptr.hpp>
#include <vector>
#include <string>
#include <QFile>
#include <QMessageBox>

class EvernoteServiceManager::EvernoteDataHolder
{
   public:
    EvernoteDataHolder() :
        m_httpClientPtr(),
        m_binaryProtocolPtr(),
        m_userStoreClientPtr(nullptr),
        m_noteStoreClientPtr(nullptr),
        m_notebookPtr(nullptr),
        m_trashNotebookPtr(nullptr),
        m_favouriteTagPtr(nullptr),
        m_noteStoreUrl("")
    {}

    boost::shared_ptr<apache::thrift::transport::THttpClient> & httpClientPtr()
    {
        return m_httpClientPtr;
    }

    boost::shared_ptr<apache::thrift::protocol::TBinaryProtocol> & binaryProtocolPtr()
    {
        return m_binaryProtocolPtr;
    }

    evernote::edam::UserStoreClient *& userStoreClientPtr()
    {
        return m_userStoreClientPtr;
    }

    evernote::edam::NoteStoreClient *& noteStoreClientPtr()
    {
        return m_noteStoreClientPtr;
    }

    evernote::edam::Notebook *& notebookPtr()
    {
        return m_notebookPtr;
    }

    evernote::edam::Notebook *& trashNotebookPtr()
    {
        return m_trashNotebookPtr;
    }

    evernote::edam::Tag *& favouriteTagPtr()
    {
        return m_favouriteTagPtr;
    }

    std::string & noteStoreUrl()
    {
        return m_noteStoreUrl;
    }

private:
    boost::shared_ptr<apache::thrift::transport::THttpClient> m_httpClientPtr;
    boost::shared_ptr<apache::thrift::protocol::TBinaryProtocol> m_binaryProtocolPtr;
    evernote::edam::UserStoreClient * m_userStoreClientPtr;
    evernote::edam::NoteStoreClient * m_noteStoreClientPtr;
    evernote::edam::Notebook * m_notebookPtr;
    evernote::edam::Notebook * m_trashNotebookPtr;
    evernote::edam::Tag * m_favouriteTagPtr;
    std::string m_noteStoreUrl;
};

void EvernoteServiceManager::authenticate()
{
    Q_CHECK_PTR(m_pOAuthHandler);

    QString errorMessage;
    if (!m_pOAuthHandler->getAccess(errorMessage)) {
        emit statusTextUpdate(errorMessage, 0);
    }
}

void EvernoteServiceManager::connect()
{
    QString errorMessage;
    if (!CheckAuthenticationState(errorMessage)) {
        emit statusTextUpdate(QString(tr("Unable to connect to Evernote - authentication failed: ")) +
                        errorMessage, 0);
        return;
    }

    std::string username = m_credentials.GetUsername().toStdString();
    std::string password = m_credentials.GetPassword().toStdString();
    std::string consumerKey = m_credentials.GetConsumerKey().toStdString();
    std::string consumerSecret = m_credentials.GetConsumerSecret().toStdString();
    std::string authToken = m_credentials.GetOAuthKey().toStdString();
    std::string authTokenSecret = m_credentials.GetOAuthSecret().toStdString();

    std::string hostname = GetHostName().toStdString();
    std::string evernoteUser = "/edam/user";

    typedef apache::thrift::transport::THttpClient THttpClient;
    boost::shared_ptr<THttpClient> authClientPtr(new THttpClient(hostname, 80, evernoteUser));
    if (authClientPtr.get() == nullptr) {
        emit statusTextUpdate(QString(tr("Unable to connect to Evernote: cannot instantiate THttp client. ")) +
                        QString(tr("Please contact application developer.")), 0);
        return;
    }

    authClientPtr->open();

    typedef apache::thrift::protocol::TBinaryProtocol TBinaryProtocol;
    boost::shared_ptr<TBinaryProtocol> userStoreProtocolPtr(new TBinaryProtocol(authClientPtr));
    if (userStoreProtocolPtr.get() == nullptr) {
        emit statusTextUpdate(QString(tr("Unable to connect to Evernote: cannot instantiate TBinaryProtocol. ")) +
                        QString(tr("Please contact application developer.")), 0);
        return;
    }

    typedef evernote::edam::UserStoreClient UserStoreClient;
    UserStoreClient userStoreClient(userStoreProtocolPtr, userStoreProtocolPtr);
    std::string noteStoreUrl = "";
    userStoreClient.getNoteStoreUrl(noteStoreUrl, authToken);

    authClientPtr->close();

    if (m_pEvernoteDataHolder == nullptr) {
        emit statusTextUpdate(QString(tr("Unable to connect to Evernote: data holder ptr is null! ")) +
                        QString(tr("Please contact application developer.")), 0);
        return;
    }

    m_pEvernoteDataHolder->noteStoreUrl() = noteStoreUrl;
    QString path = m_pEvernoteDataHolder->noteStoreUrl().c_str();
    path.replace("https://","");
    path.replace(hostname.c_str(),"");

    typedef apache::thrift::transport::TSSLSocketFactory TSSLSocketFactory;
    boost::shared_ptr<TSSLSocketFactory> factoryPtr(new TSSLSocketFactory);
    if (factoryPtr.get() == nullptr) {
        emit statusTextUpdate(QString(tr("Unable to connect to Evernote: unable to instantiate TSSLSocketFactory. ")) +
                        QString(tr("Please contact application developer.")), 0);
        return;
    }

    typedef apache::thrift::transport::TSSLSocket TSSLSocket;
    boost::shared_ptr<TSSLSocket> socketPtr = factoryPtr->createSocket();
    if (socketPtr.get() == nullptr) {
        emit statusTextUpdate(QString(tr("Unable to connect to Evernote: unable to instantiate TSSLSocket. ")) +
                        QString(tr("Please contact application developer.")), 0);
        return;
    }

    // TODO: make it adjustable at runtime
    socketPtr->setConnTimeout(12000);
    socketPtr->setRecvTimeout(12000);
    socketPtr->setSendTimeout(12000);

    typedef apache::thrift::transport::TBufferedTransport TBufferedTransport;
    boost::shared_ptr<TBufferedTransport> transportPtr(new TBufferedTransport(socketPtr));
    if (transportPtr.get() == nullptr) {
        emit statusTextUpdate(QString(tr("Unable to connect to Evernote: unable to instantiate TBufferedTransport. ")) +
                        QString(tr("Please contact application developer.")), 0);
        return;
    }

    boost::shared_ptr<THttpClient> & httpClientPtr = m_pEvernoteDataHolder->httpClientPtr();
    httpClientPtr.reset(new THttpClient(transportPtr, hostname, path.toUtf8().constData()));
    if (httpClientPtr.get() == nullptr) {
        emit statusTextUpdate(QString(tr("Unable to connect to Evernote: unable to instantiate THttpClient. ")) +
                        QString(tr("Please contact application developer.")), 0);
        return;
    }

    httpClientPtr->open();
    m_pEvernoteDataHolder->binaryProtocolPtr().reset(new TBinaryProtocol(httpClientPtr));

    typedef evernote::edam::NoteStoreClient NoteStoreClient;
    NoteStoreClient *& pNoteStoreClient = m_pEvernoteDataHolder->noteStoreClientPtr();
    if (pNoteStoreClient != nullptr) {
        delete pNoteStoreClient;
        pNoteStoreClient = nullptr;
    }

    pNoteStoreClient = new NoteStoreClient(m_pEvernoteDataHolder->binaryProtocolPtr(),
                                           m_pEvernoteDataHolder->binaryProtocolPtr());
    if (pNoteStoreClient == nullptr) {
        emit statusTextUpdate(QString(tr("Unable to connect to Evernote: unable to instantiate NoteStoreClient. ")) +
                        QString(tr("Please contact application developer.")), 0);
        return;
    }

    SetDefaultNotebook();
    SetTrashNotebook();
    SetFavouriteTag();
    SetConnectionState(ECS_CONNECTED);
    // TODO: set up timer to refresh connection, synchronize
}

void EvernoteServiceManager::disconnect()
{
    if (m_pEvernoteDataHolder == nullptr) {
        return;
    }

    typedef apache::thrift::transport::THttpClient THttpClient;
    boost::shared_ptr<THttpClient> & httpClientPtr = m_pEvernoteDataHolder->httpClientPtr();
    if (httpClientPtr.get() == nullptr) {
        return;
    }
    else {
        httpClientPtr->flush();
        httpClientPtr->close();
        SetConnectionState(ECS_DISCONNECTED);
    }
}

void EvernoteServiceManager::setRefreshTime(const double refreshTime)
{
    m_refreshTime = refreshTime;
}

EvernoteServiceManager::EvernoteServiceManager() :
    m_pOAuthHandler(new EvernoteServiceOAuthHandler(this)),
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
    typedef evernote::edam::Notebook Notebook;
    std::vector<Notebook> notebooks;

    if (m_pEvernoteDataHolder == nullptr) {
        emit statusTextUpdate(QString(tr("Unable to set default notebook: EvernoteDataHolder is null. ")) +
                        QString(tr("Please contact application developer.")), 0);
        return;
    }

    typedef evernote::edam::NoteStoreClient NoteStoreClient;
    NoteStoreClient * noteStoreClientPtr = m_pEvernoteDataHolder->noteStoreClientPtr();
    if (noteStoreClientPtr == nullptr) {
        emit statusTextUpdate(QString(tr("Unable to set default notebook: NoteStoreClient is null. ")) +
                        QString(tr("Please contact application developer.")), 0);
        return;
    }

    std::string authToken = m_credentials.GetOAuthKey().toStdString();
    noteStoreClientPtr->listNotebooks(notebooks, authToken);

    Notebook defaultNotebook;
    noteStoreClientPtr->getDefaultNotebook(defaultNotebook, authToken);

    Notebook *& notebookPtr = m_pEvernoteDataHolder->notebookPtr();
    size_t numNotebooks = notebooks.size();
    for(size_t i = 0; i < numNotebooks; ++i)
    {
        Notebook & notebook = notebooks[i];
        if (notebook.guid == defaultNotebook.guid)
        {   
            if (notebookPtr != nullptr) {
                delete notebookPtr;
                notebookPtr = nullptr;
            }
            notebookPtr = new Notebook(notebook);
            return;
        }
    }

    // Default notebook not found, creating one
    noteStoreClientPtr->createNotebook(defaultNotebook, authToken, defaultNotebook);
    noteStoreClientPtr->getNotebook(defaultNotebook, authToken, defaultNotebook.guid);

    if (notebookPtr != nullptr) {
        delete notebookPtr;
        notebookPtr = nullptr;
    }
    notebookPtr = new Notebook(defaultNotebook);
}

void EvernoteServiceManager::SetTrashNotebook()
{
    typedef evernote::edam::Notebook Notebook;
    std::vector<Notebook> notebooks;

    if (m_pEvernoteDataHolder == nullptr) {
        emit statusTextUpdate(QString(tr("Unable to set default notebook: EvernoteDataHolder is null. ")) +
                        QString(tr("Please contact application developer.")), 0);
        return;
    }

    typedef evernote::edam::NoteStoreClient NoteStoreClient;
    NoteStoreClient * noteStoreClientPtr = m_pEvernoteDataHolder->noteStoreClientPtr();
    if (noteStoreClientPtr == nullptr) {
        emit statusTextUpdate(QString(tr("Unable to set default notebook: NoteStoreClient is null. ")) +
                        QString(tr("Please contact application developer.")), 0);
        return;
    }

    std::string authToken = m_credentials.GetOAuthKey().toStdString();

    Notebook *& trashNotebookPtr = m_pEvernoteDataHolder->trashNotebookPtr();
    if (trashNotebookPtr == nullptr)
    {
        trashNotebookPtr = new Notebook;
        trashNotebookPtr->name = QString(tr("Trash notebook")).toStdString();
        noteStoreClientPtr->createNotebook(*trashNotebookPtr, authToken,
                                           *trashNotebookPtr);
    }
    else
    {
        noteStoreClientPtr->listNotebooks(notebooks, authToken);

        size_t numNotebooks = notebooks.size();
        for(size_t i = 0; i < numNotebooks; ++i)
        {
            Notebook & notebook = notebooks[i];
            if (notebook.guid == trashNotebookPtr->guid)
            {
                delete trashNotebookPtr;
                trashNotebookPtr = new Notebook(notebook);
                return;
            }
        }

        // Trash notebook not found, creating one
        delete trashNotebookPtr;
        trashNotebookPtr = new Notebook;
        trashNotebookPtr->name = QString(tr("Trash notebook")).toStdString();
        noteStoreClientPtr->createNotebook(*trashNotebookPtr, authToken,
                                           *trashNotebookPtr);
    }
}

void EvernoteServiceManager::SetFavouriteTag()
{
    typedef evernote::edam::Tag Tag;
    std::vector<Tag> tags;

    if (m_pEvernoteDataHolder == nullptr) {
        emit statusTextUpdate(QString(tr("Unable to set favourite tag: EvernoteDataHolder is null. ")) +
                        QString(tr("Please contact application developer.")), 0);
        return;
    }

    typedef evernote::edam::NoteStoreClient NoteStoreClient;
    NoteStoreClient  * noteStoreClientPtr = m_pEvernoteDataHolder->noteStoreClientPtr();
    if (noteStoreClientPtr == nullptr) {
        emit statusTextUpdate(QString(tr("Unable to set favourite tag: NoteStoreClient is null. ")) +
                        QString(tr("Please contact application developer.")), 0);
        return;
    }

    std::string authToken = m_credentials.GetOAuthKey().toStdString();

    Tag *& favouriteTagPtr = m_pEvernoteDataHolder->favouriteTagPtr();
    if (favouriteTagPtr == nullptr)
    {
        favouriteTagPtr = new Tag;
        favouriteTagPtr->name = QString(tr("Favourite tag")).toStdString();
        noteStoreClientPtr->createTag(*favouriteTagPtr, authToken, *favouriteTagPtr);
        return;
    }
    else
    {
        noteStoreClientPtr->listTags(tags, authToken);

        size_t numTags = tags.size();
        for(size_t i = 0; i < numTags; ++i)
        {
            Tag & tag = tags[i];
            if (tag.guid == favouriteTagPtr->guid)
            {
                delete favouriteTagPtr;
                favouriteTagPtr = new Tag(tag);
                return;
            }
        }

        // Favourite tag not found, creating one
        delete favouriteTagPtr;
        favouriteTagPtr = new Tag;
        favouriteTagPtr->name = QString(tr("Favourite tag")).toStdString();
        noteStoreClientPtr->createTag(*favouriteTagPtr, authToken, *favouriteTagPtr);
    }
}

EvernoteServiceManager & EvernoteServiceManager::Instance()
{
    return Singleton<EvernoteServiceManager>::Instance();
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

    tokensFile.write(encryptedKey.toAscii());
    tokensFile.write(encryptedSecret.toAscii());
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
}

void EvernoteServiceManager::onUserNameAndPasswordSet(QString name, QString password)
{
    m_credentials.SetUsername(name);
    m_credentials.SetPassword(password);
}
