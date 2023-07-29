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
    VK_Manager(const QString&, const QString&, const QString&);
    QString prefix() const { return QString("photo-%1_").arg(group_id); };

signals:
    void albums_ready(const QMap<QString, int>&);
    void photo_ids_ready(const QVector<int>&, const QStringList&);
    void image_ready(const QImage&);
    void posted_successfully(int, int);
    void post_failed(int, const QString&);

public slots:
    QNetworkReply* get_url(const QString& url);
    void get_albums();
    void get_photo_ids(int album_id, const QString& photo_ids = "");
    void get_image(const QString& url);
    void post(int, const QString&, int);

private slots:
    QJsonObject reply_json(QNetworkReply *response);
    QString link(const QJsonObject & photo_item);
    QImage image(QNetworkReply *response);
    void got_albums(QNetworkReply *);
    void got_photo_ids(QNetworkReply *);
    void got_image(QNetworkReply *);

private:
    const QString access_token;
    const QString group_id;
    const QString public_id;
};

#endif // VK_MANAGER_H
