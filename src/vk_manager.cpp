#include "vk_manager.h"
#include <QRegularExpression>

VK_Manager::VK_Manager(const QString& access_token, const QString& group_id, const QString& public_id)
    : QNetworkAccessManager()
    , access_token(access_token)
    , group_id(group_id)
    , public_id(public_id) {}

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
    response->deleteLater();
    if (response->error() != QNetworkReply::NoError) return QImage();
    QImageReader reader(response);
    QImage loaded_image = reader.read();
    return loaded_image;
}

QNetworkReply* VK_Manager::get_url(const QString& url) {
    return get(QNetworkRequest(QUrl(url)));
}

void VK_Manager::get_photo_ids(int album_id, const QString& photo_ids) {
    QString url = "https://api.vk.com/method/photos.get?v=5.131"
                  "&access_token=" + access_token
                + "&owner_id=-" + group_id
                + "&album_id=" + QString().setNum(album_id)
                + "&count=400"
                + (!photo_ids.isEmpty() ? "&photo_ids=" : "") + photo_ids;
    get_url(url);
    connect(this, &QNetworkAccessManager::finished, this, &VK_Manager::got_photo_ids);
}

void VK_Manager::get_image(const QString& url) {
    connect(this, &QNetworkAccessManager::finished, this, &VK_Manager::got_image);
    get_url(url);
}

void VK_Manager::get_albums() {
    QString url = "https://api.vk.com/method/photos.getAlbums?v=5.131"
                  "&access_token=" + access_token
                + "&owner_id=-" + group_id;
    get_url(url);
    connect(this, &QNetworkAccessManager::finished, this, &VK_Manager::got_albums);
}

void VK_Manager::post(int index, const QString& attachments, int date) {
    QString url = "https://api.vk.com/method/wall.post?v=5.131&from_group=1&signed=0"
                  "&access_token=" + access_token
                + "&owner_id=-" + public_id
                + "&attachments=" + attachments
                + "&publish_date=" + QString().setNum(date);
    auto response = get_url(url);
    connect(response, &QNetworkReply::finished, [this, response, index, date](){
        auto reply = reply_json(response);
        response->deleteLater();
        if (reply.contains("error")) {
            qDebug() << reply;
            emit post_failed(index, QJsonDocument(reply).toJson(QJsonDocument::Compact));
            return;
        }
        emit posted_successfully(index, date);
    });
}

void VK_Manager::get_poll(const QString& options, int end_date) {
    QString url = "https://api.vk.com/method/polls.create?v=5.199&from_group=1&signed=0"
                  "&access_token=" + access_token
                + "&owner_id=-" + public_id
                + "&question=" + "Тема пятничных постов"
                + "&is_anonymous=1&is_multiple=1&background_id=4"
                + "&add_answers=" + options
                + "&end_date=" + QString().setNum(end_date);
    auto response = get_url(url);
    connect(response, &QNetworkReply::finished, [this, response](){
        auto reply = reply_json(response);
        response->deleteLater();
        if (reply.contains("error")) {
            emit poll_post_failed(QJsonDocument(reply).toJson(QJsonDocument::Compact));
            return;
        }
        int poll_id = reply["response"].toObject()["id"].toInt();
        emit poll_ready(poll_id);
    });
}

void VK_Manager::post(const QString& message, int poll_id, int date) {
    QString url = "https://api.vk.com/method/wall.post?v=5.199&from_group=1&signed=0"
                  "&access_token=" + access_token
                + "&owner_id=-" + public_id
                + "&message=" + message
                + "&attachments=" + poll_attachment(poll_id)
                + "&publish_date=" + QString().setNum(date);
    auto response = get_url(url);
    connect(response, &QNetworkReply::finished, [this, response](){
        auto reply = reply_json(response);
        response->deleteLater();
        if (reply.contains("error")) {
            emit poll_post_failed(QJsonDocument(reply).toJson(QJsonDocument::Compact));
            return;
        }
        emit poll_posted_successfully();
    });
}

void VK_Manager::edit_photo_caption(int photo_id, const QString& caption, const QString& captcha_sid, const QString& captcha_key) {
    QString url = "https://api.vk.com/method/photos.edit?v=5.131"
                  "&access_token=" + access_token
                + "&owner_id=-" + group_id
                + "&photo_id=" + QString().setNum(photo_id)
                + "&caption=" + caption;
    if (!captcha_sid.isEmpty()) {
        url += "&captcha_sid=" + captcha_sid + "&captcha_key=" + captcha_key;
    }
    auto response = get_url(url);
    connect(response, &QNetworkReply::finished, [this, response](){
        auto reply = reply_json(response);
        response->deleteLater();
        if (reply.contains("error")) {
            auto error = reply["error"].toObject();
            qDebug() << error;
            get_image(error["captcha_img"].toString());
            emit captcha_error(error["captcha_sid"].toString());
        } else {
            emit caption_passed();
        }
    });
}

void VK_Manager::got_albums(QNetworkReply *response) {
    disconnect(this, &QNetworkAccessManager::finished, this, &VK_Manager::got_albums);
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
    emit albums_ready(album_ids);
}

void VK_Manager::got_photo_ids(QNetworkReply *response) {
    disconnect(this, &QNetworkAccessManager::finished, this, &VK_Manager::got_photo_ids);
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
    emit photo_ids_ready(photo_ids, links);
}

void VK_Manager::got_image(QNetworkReply *response) {
    disconnect(this, &QNetworkAccessManager::finished, this, &VK_Manager::got_image);
    emit image_ready(image(response));
}
