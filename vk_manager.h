#ifndef VK_MANAGER_H
#define VK_MANAGER_H

#include <QNetworkAccessManager>
#include <QNetworkReply>
#include <QJsonObject>
#include <QJsonDocument>
#include <QJsonArray>
#include <QImageReader>

class VK_Manager : public QNetworkAccessManager
{
    Q_OBJECT
public:
    VK_Manager();

signals:
    void albums_ready(const QMap<QString, int>&);
    void photo_ids_ready(const QVector<int>&, const QStringList&);
    void image_ready(const QImage&);

public slots:
    QNetworkReply* get_url(const QString& url);
    void get_albums();
    void get_photo_ids(int album_id, const QString& photo_ids = "");
    void get_image(const QString& url);
    void set_access_token(const QString&);
//    void get_access_token(int client_id);
//    QString current_token() const { return access_token; }

private slots:
    QJsonObject reply_json(QNetworkReply *response);
    QString link(const QJsonObject & photo_item);
    QImage image(QNetworkReply *response);
    void got_albums(QNetworkReply *);
    void got_photo_ids(QNetworkReply *);
    void got_image(QNetworkReply *);
//    void access_token_ready(QNetworkReply *);

private:
    QString access_token;
    const QString group_id = "42265360";
};

#endif // VK_MANAGER_H
