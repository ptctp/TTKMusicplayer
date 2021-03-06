#include "musicnetworkoperator.h"
#include "musicdownloadsourcethread.h"
#include "musicobject.h"
#///QJson import
#include "qjson/parser.h"

#define IP_CHECK_URL    "ZlhkcnFzd1RabVhCZXNWM1pnbk5hT1ErL2RpMUNjK0hYQ3FXUHdCOVNGSlpDU2ZmNTZnekhHUlo3WkwrUWhtQXljNitUcjJmZ0RId004OFc5QlVibjhvRGlRSzY3QXlVbmZHNFV3bkhZdGZMU2JTZ3lJTkNhOGZJUlNhcmlBUmcvRUVrQWc9PQ=="

MusicNetworkOperator::MusicNetworkOperator(QObject *parent)
    : QObject(parent)
{

}

void MusicNetworkOperator::startToDownload()
{
    MusicDownloadSourceThread *download = new MusicDownloadSourceThread(this);
    connect(download, SIGNAL(downLoadByteDataChanged(QByteArray)), SLOT(downLoadFinished(QByteArray)));
    download->startToDownload(MusicUtils::Algorithm::mdII(IP_CHECK_URL, false));
}

void MusicNetworkOperator::downLoadFinished(const QByteArray &data)
{
    QString line;

    QJson::Parser parser;
    bool ok;
    const QVariant &json = parser.parse(data, &ok);
    if(ok)
    {
        QVariantMap value = json.toMap();
        if(value.contains("result"))
        {
            value = value["result"].toMap();
            line = value["operators"].toString();
        }
    }

    Q_EMIT getNetworkOperatorFinished(line);
    deleteLater();
}
