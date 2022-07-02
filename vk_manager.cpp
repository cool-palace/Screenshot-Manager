#include "vk_manager.h"
#include <QRegularExpression>

VK_Manager::VK_Manager(const QString& access_token) :
    QNetworkAccessManager(),
    access_token(access_token) { }

void VK_Manager::get_url(const QString& url) {
    get(QNetworkRequest(QUrl(url)));
}

void VK_Manager::get_photo_ids(int album_id, const QString& photo_ids) {
    QString url = "https://api.vk.com/method/photos.get?v=5.131"
                  "&access_token=" + access_token
                + "&owner_id=-" + group_id
                + "&album_id=" + QString().setNum(album_id)
                + "&count=255"
                + (!photo_ids.isEmpty() ? "&photo_ids=" : "") + photo_ids;
    get_url(url);
    connect(this, &QNetworkAccessManager::finished, this, &VK_Manager::photo_ids_ready);
}

void VK_Manager::get_photo(int photo_id) {
    QString url = "https://api.vk.com/method/photos.getById?v=5.131"
                  "&access_token=" + access_token
                + "&photos=-" + group_id + '_' + QString().setNum(photo_id)
                + "&photo_sizes=1";
    get_url(url);
    connect(this, &QNetworkAccessManager::finished, this, &VK_Manager::photo_ready);
}

void VK_Manager::get_albums() {
    QString url = "https://api.vk.com/method/photos.getAlbums?v=5.131"
                  "&access_token=" + access_token
                + "&owner_id=-" + group_id;
    get_url(url);
    connect(this, &QNetworkAccessManager::finished, this, &VK_Manager::albums_ready);
}

void VK_Manager::get_access_token(int client_id) {
    QString url = "https://oauth.vk.com/authorize"
                  "?client_id=" + QString().setNum(client_id) +
                  "?display=page&scope=groups"
                  "&response_type=token&v=5.131&state=1330"
                  "method/photos.getAlbums?v=5.131";
    get_url(url);
    connect(this, &QNetworkAccessManager::finished, this, &VK_Manager::access_token_ready);
}

void VK_Manager::access_token_ready(QNetworkReply* response) {
    response->deleteLater();
    if (response->error() != QNetworkReply::NoError) return;
    auto url = response->url().url();
    QRegularExpression regex("access_token=(.+?)&");
    auto match = regex.match(url);
    if (match.hasMatch()) {
        access_token = match.captured(1);
    }
    disconnect(this, &QNetworkAccessManager::finished, this, &VK_Manager::access_token_ready);
    get_albums();
}

void VK_Manager::set_access_token(const QString& token) {
    access_token = token;
}
