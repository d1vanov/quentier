
#include "AsyncResult.h"
#include <typeinfo>
#include "http.h"

qevercloud::AsyncResult::AsyncResult(QString url, QByteArray postData, qevercloud::AsyncResult::ReadFunctionType readFunction, bool autoDelete, QObject *parent)
    : QObject(parent), url_(url), postData_(postData), readFunction_(readFunction), autoDelete_(autoDelete)
{
    QMetaObject::invokeMethod(this, "start", Qt::QueuedConnection);
}

void qevercloud::AsyncResult::start()
{
    ReplyFetcher* f = new ReplyFetcher;
    QObject::connect(f, SIGNAL(replyFetched(qevercloud::ReplyFetcher*)), this, SLOT(onReplyFetched(QObject*)));
    f->start(evernoteNetworkAccessManager(), createEvernoteRequest(url_), postData_);
}

void qevercloud::AsyncResult::onReplyFetched(QObject *rp)
{
    ReplyFetcher* reply = qobject_cast<ReplyFetcher*>(rp);
    QSharedPointer<EverCloudExceptionData> error;
    QVariant result;
    try {
        if(reply->isError()) {
            error = QSharedPointer<EverCloudExceptionData>(new EverCloudExceptionData(reply->errorText()));
        } else if(reply->httpStatusCode() != 200) {
            error = QSharedPointer<EverCloudExceptionData>(new EverCloudExceptionData(QStringLiteral("HTTP Status Code = %1").arg(reply->httpStatusCode())));
        } else {
            result = readFunction_(reply->receivedData());
        }
    } catch(const EverCloudException& e) {
        error = e.exceptionData();
    } catch(const std::exception& e) {
        error = QSharedPointer<EverCloudExceptionData>(new EverCloudExceptionData(QStringLiteral("Exception of type \"%1\" with the message: %2").arg(typeid(e).name()).arg(e.what())));
    } catch(...) {
        error = QSharedPointer<EverCloudExceptionData>(new EverCloudExceptionData(QStringLiteral("Unknown exception")));
    }
    emit finished(result, error);
    reply->deleteLater();
    if(autoDelete_) this->deleteLater();
}

