#ifndef VK_MANAGER_H
#define VK_MANAGER_H

#include <QNetworkAccessManager>
#include <QNetworkReply>
#include <QJsonObject>
#include <QJsonDocument>
#include <QJsonArray>
#include <QImageReader>
#include <QThread>
#include <QFile>

class VK_Manager : public QNetworkAccessManager
{
    Q_OBJECT
private:
    VK_Manager();
public:
    static VK_Manager& instance();
    VK_Manager(const VK_Manager&) = delete;
    VK_Manager& operator=(const VK_Manager&) = delete;
    ~VK_Manager();
    void init(const QString& access_token, const QString& group_id, const QString& public_id);
    QString prefix() const { return QString("photo-%1_").arg(m_group_id); };
    QString poll_attachment(int id) const { return QString("poll-%1_%2").arg(m_public_id).arg(id); };

signals:
    void albums_ready(const QMap<QString, int>&);
    void photo_ids_ready(const QVector<int>&, const QStringList&);
    void image_ready(const QImage&);
    void posted_successfully(int, int);
    void poll_posted_successfully();
    void post_failed(int, const QString&);
    void poll_post_failed(const QString&);
    void poll_ready(int);
    void caption_passed();
    void captcha_error(const QString&, const QString&);
    void captcha_image_ready(const QImage&);

public slots:
    QNetworkReply* get_url(const QString& url);
    void get_albums();
    void get_photo_ids(int album_id, const QString& photo_ids = "");
    void get_image(const QString& url);
    void post(int, const QString&, int);
    void post(const QString&, int, int);
    void get_poll(const QString&, int);
    void edit_photo_caption(int, const QString&, const QString& captcha_sid = "", const QString& captcha_key = "");
    void get_captcha(const QString& url);

private slots:
    QJsonObject reply_json(QNetworkReply *response);
    QString link(const QJsonObject & photo_item);
    QImage image(QNetworkReply *response);
    void got_albums(QNetworkReply *);
    void got_photo_ids(QNetworkReply *);
    void got_image(QNetworkReply *);
    void got_captcha(QNetworkReply *);

private:
    QString m_access_token;
    QString m_group_id;
    QString m_public_id;
};

#endif // VK_MANAGER_H
