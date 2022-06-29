#ifndef VK_MANAGER_H
#define VK_MANAGER_H

#include <QNetworkAccessManager>
#include <QNetworkReply>
#include <QJsonObject>
#include <QJsonDocument>

class VK_Manager : public QNetworkAccessManager
{
    Q_OBJECT
public:
    VK_Manager(const QString& access_token = "");

signals:
    void albums_ready(QNetworkReply *);
    void photos_ready(QNetworkReply *);
    void image_ready(QNetworkReply *);

public slots:
    void get_url(const QString& url);
    void get_photos(int album_id, const QString& photo_ids = "");
    void get_albums();
    void get_access_token(int client_id);

private slots:
    void access_token_ready(QNetworkReply *);

private:
    QString access_token;
    const QString group_id = "42265360";
};

#endif // VK_MANAGER_H
