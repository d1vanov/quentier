#ifndef __QUTE_NOTE__EVERNOTE_CLIENT__NOTE_STORE_IMPL_H
#define __QUTE_NOTE__EVERNOTE_CLIENT__NOTE_STORE_IMPL_H

#include "../evernote_client/NoteStore.h"
#include "../../../thrift/src/thrift/protocol/TBinaryProtocol.h"
#include <boost/shared_ptr.hpp>

namespace apache {
namespace thrift {

namespace transport {
QT_FORWARD_DECLARE_CLASS(THttpClient)
}

} // namespace thrift
} // namespace apache

namespace evernote {
namespace edam {

QT_FORWARD_DECLARE_CLASS(NoteStoreClient)

} // namespace edam
} // namespace evernote

namespace qute_note {

class NoteStorePrivate final
{
public:
    NoteStorePrivate(const QString & authenticationToken,
                     const QString & host, const int port,
                     const QString & noteStorePath);
    ~NoteStorePrivate();

    std::string m_authToken;
    std::string m_host;
    int m_port;
    std::string m_noteStorePath;

    boost::shared_ptr<apache::thrift::transport::THttpClient>    m_pHttpClient;
    boost::shared_ptr<apache::thrift::protocol::TBinaryProtocol> m_pBinaryProtocol;
    QScopedPointer<evernote::edam::NoteStoreClient> m_pNoteStoreClient;
};

}

#endif // __QUTE_NOTE__EVERNOTE_CLIENT__NOTE_STORE_IMPL_H
