#include "vk_manager.h"

VK_Manager::VK_Manager(const QString& access_token) :
    QNetworkAccessManager(),
    access_token(access_token) { }

void VK_Manager::get_url(const QString& url) {
    get(QNetworkRequest(QUrl(url)));
}

void VK_Manager::get_photos(const QString& album_id, const QString& photo_ids) {
    QString url = "https://api.vk.com/method/photos.get?v=5.131"
                  "&access_token=" + access_token
                + "&owner_id=-" + group_id
                + "&album_id=" + album_id
                + (!photo_ids.isEmpty() ? "&photo_ids=" : "") + photo_ids;
    get_url(url);
    connect(this, &QNetworkAccessManager::finished, this, &VK_Manager::photos_ready);
}

void VK_Manager::get_albums() {
    QString url = "https://api.vk.com/method/photos.getAlbums?v=5.131"
                  "&access_token=" + access_token
                + "&owner_id=-" + group_id;
    get_url(url);
    connect(this, &QNetworkAccessManager::finished, this, &VK_Manager::albums_ready);
}

//QSharedPointer<QJsonObject> reply(QNetworkReply *response) {
//    response->deleteLater();
//    if (response->error() != QNetworkReply::NoError) return QSharedPointer<QJsonObject>();
//    QJsonObject * reply = new QJsonObject(QJsonDocument::fromJson(response->readAll()).object());
//    return QSharedPointer<QJsonObject>::create(reply);
//}
