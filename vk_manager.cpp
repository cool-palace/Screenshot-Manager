#include "vk_manager.h"
#include <QRegularExpression>

VK_Manager::VK_Manager() :
    QNetworkAccessManager() { }

QJsonObject VK_Manager::reply_json(QNetworkReply *response) {
    response->deleteLater();
    if (response->error() != QNetworkReply::NoError) return QJsonObject();
    auto reply = QJsonDocument::fromJson(response->readAll()).object();
    return reply;
}

QString VK_Manager::link(const QJsonObject & photo_item) {
    auto array = photo_item["sizes"].toArray();
    QString url;
    for (QJsonValueRef item : array) {
        if (item.toObject()["type"].toString() == "z") {
            url = item.toObject()["url"].toString();
            break;
        }
    }
    if (url.isEmpty()) {
        for (QJsonValueRef item : array) {
            if (item.toObject()["type"].toString() == "y") {
                url = item.toObject()["url"].toString();
                break;
            }
        }
    }
    return url;
}

QImage VK_Manager::image(QNetworkReply *response) {
    QImageReader reader(response);
    QImage loaded_image = reader.read();
    return loaded_image;
}

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

void VK_Manager::get_photo(const QString& url) {
    connect(this, &QNetworkAccessManager::finished, this, &VK_Manager::image_ready);
    get_url(url);
}

void VK_Manager::get_albums() {
    QString url = "https://api.vk.com/method/photos.getAlbums?v=5.131"
                  "&access_token=" + access_token
                + "&owner_id=-" + group_id;
    get_url(url);
    connect(this, &QNetworkAccessManager::finished, this, &VK_Manager::got_albums);
}

//void VK_Manager::get_access_token(int client_id) {
//    QString url = "https://oauth.vk.com/authorize"
//                  "?client_id=" + QString().setNum(client_id) +
//                  "?display=page&scope=groups,photos,offline"
//                  "&response_type=token&v=5.131&state=1330";
//    get_url(url);
//    connect(this, &QNetworkAccessManager::finished, this, &VK_Manager::access_token_ready);
//}

//void VK_Manager::access_token_ready(QNetworkReply* response) {
//    disconnect(this, &QNetworkAccessManager::finished, this, &VK_Manager::access_token_ready);
//    response->deleteLater();
//    if (response->error() != QNetworkReply::NoError) return;
//    auto url = response->url().url();
//    QRegularExpression regex("access_token=(.+?)&");
//    auto match = regex.match(url);
//    if (match.hasMatch()) {
//        access_token = match.captured(1);
//    }
//}

//void VK_Manager::set_access_token(const QString& token) {
//    access_token = token;
//}

void VK_Manager::got_albums(QNetworkReply *response) {
    disconnect(this, &QNetworkAccessManager::finished, this, &VK_Manager::albums_ready);
    auto reply = reply_json(response);
    if (reply.contains("error")) return;
    QMap<QString, int> album_ids;
    auto items = reply["response"].toObject()["items"].toArray();
    for (const QJsonValueRef album : items) {
        auto album_object = album.toObject();
        auto title = album_object["title"].toString();
        int album_id = album_object["id"].toInt();
        album_ids.insert(title, album_id);
    }
    emit albums_ready(std::move(album_ids));
}

void VK_Manager::got_photo_ids(QNetworkReply *response) {
    disconnect(this, &QNetworkAccessManager::finished, this, &VK_Manager::photo_ids_ready);
    auto reply = reply_json(response);
    QVector<int> photo_ids;
    QStringList links;
    auto array = reply["response"].toObject()["items"].toArray();
    for (const QJsonValueRef item : array) {
        auto current_item = item.toObject();
        auto id = current_item["id"].toInt();
        photo_ids.push_back(id);
        links.push_back(link(current_item));
    }
    emit photo_ids_ready(std::move(photo_ids), std::move(links));
}

void VK_Manager::got_image(QNetworkReply *response) {
    disconnect(this, &QNetworkAccessManager::finished, this, &VK_Manager::image_ready);
    emit image_ready(image(response));
}

