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
    void photo_ids_ready(QNetworkReply *);
    void photo_ready(QNetworkReply *);
    void image_ready(QNetworkReply *);

public slots:
    void get_url(const QString& url);
    void get_photo_ids(int album_id, const QString& photo_ids = "");
    void get_photo(int photo_id);
    void get_photo(const QString& url);
    void get_albums();
    void get_access_token(int client_id);
    void set_access_token(const QString&);
    QString current_token() const { return access_token; }

private slots:
    void access_token_ready(QNetworkReply *);

private:
    QString access_token;
    const QString group_id = "42265360";
};

#endif // VK_MANAGER_H
