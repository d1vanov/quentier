#include "NoteStore_p.h"
#include "../../../thrift/src/thrift/transport/THttpClient.h"
#include "../../../thrift/src/thrift/transport/TSSLSocket.h"
#include "../../../thrift/src/thrift/transport/TBufferTransports.h"
#include "../evernote_sdk/src/NoteStore.h"

using namespace apache::thrift::transport;
using namespace apache::thrift::protocol;

namespace qute_note {

NoteStorePrivate::NoteStorePrivate(const QString & authenticationToken,
                                   const QString & host, const int port,
                                   const QString & noteStorePath) :
    m_authToken(authenticationToken.toStdString()),
    m_host(host.toStdString()),
    m_port(port),
    m_noteStorePath(noteStorePath.toStdString()),
    m_pHttpClient(),
    m_pBinaryProtocol(),
    m_pNoteStoreClient()
{
    TSSLSocketFactory factory;
    boost::shared_ptr<TSSLSocket> socket = factory.createSocket(m_host, m_port);
    boost::shared_ptr<TBufferedTransport> transport(new TBufferedTransport(socket));

    m_pHttpClient.reset(new THttpClient(transport, m_host, m_noteStorePath.c_str()));
    m_pHttpClient->open();

    m_pBinaryProtocol.reset(new TBinaryProtocol(m_pHttpClient));
    m_pNoteStoreClient.reset(new evernote::edam::NoteStoreClient(m_pBinaryProtocol,
                                                                 m_pBinaryProtocol));
}

NoteStorePrivate::~NoteStorePrivate()
{}

}
