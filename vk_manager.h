#ifndef VK_MANAGER_H
#define VK_MANAGER_H

#include <QNetworkAccessManager>
#include <QNetworkReply>
#include <QJsonObject>
#include <QJsonDocument>

class VK_Manager : public QNetworkAccessManager
{
    Q_OBJECT
//    using Reply = QSharedPointer<QJsonObject>;
public:
    VK_Manager(const QString& access_token);

signals:
    void albums_ready(QNetworkReply *);
    void photos_ready(QNetworkReply *);
    void image_ready(QNetworkReply *);

public slots:
    void get_url(const QString& url);
    void get_photos(const QString& album_id, const QString& photo_ids = "");
    void get_albums();

private slots:
//    Reply reply(QNetworkReply *response);

private:
    const QString access_token;
    const QString group_id = "42265360";
};

#endif // VK_MANAGER_H
