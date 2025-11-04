#include "include\vk_manager.h"
#include <QRegularExpression>
#include <QTimer>
#include <QUrlQuery>

VK_Manager::VK_Manager()
    : QNetworkAccessManager() {}

VK_Manager::~VK_Manager() {}

void VK_Manager::init(const QString &access_token, const QString &group_id, const QString &public_id) {
    m_access_token = access_token;
    m_group_id = group_id;
    m_public_id = public_id;
}

VK_Manager &VK_Manager::instance() {
    static VK_Manager instance;
    return instance;
}

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
                  "&access_token=" + m_access_token
                + "&owner_id=-" + m_group_id
                + "&album_id=" + QString().setNum(album_id)
                + "&count=500"
                + (!photo_ids.isEmpty() ? "&photo_ids=" : "") + photo_ids;
    get_url(url);
    connect(this, &QNetworkAccessManager::finished, this, &VK_Manager::got_photo_ids);
}

void VK_Manager::get_image(const QString& url) {
    connect(this, &QNetworkAccessManager::finished, this, &VK_Manager::got_image);
    get_url(url);
}

void VK_Manager::get_captcha(const QString &url) {
    connect(this, &QNetworkAccessManager::finished, this, &VK_Manager::got_captcha);
    get_url(url);
}

void VK_Manager::get_posts() {
    QString url = "https://api.vk.com/method/wall.get?v=5.199"
                  "&access_token=" + m_access_token
                + "&domain=public" + m_public_id
                + "&count=" + QString::number(100)
                + "&offset=" + QString::number(m_offset);
    get_url(url);
    connect(this, &QNetworkAccessManager::finished, this, &VK_Manager::got_posts);
}

void VK_Manager::get_recent_posts(int count) {
    QString url = "https://api.vk.com/method/wall.get?v=5.199"
                  "&access_token=" + m_access_token
                + "&domain=public" + m_public_id
                + "&count=" + QString::number(count);
    get_url(url);
    connect(this, &QNetworkAccessManager::finished, this, &VK_Manager::got_recent_posts);
}

void VK_Manager::collect_posts() {
    m_result = QJsonObject();
    m_offset = 0;
    get_postponed_posts();
}

void VK_Manager::get_postponed_posts() {
    QString url = "https://api.vk.com/method/wall.get?v=5.199"
                  "&access_token=" + m_access_token
                + "&domain=public" + m_public_id
                + "&count=" + QString::number(100)
                + "&filter=postponed";
    get_url(url);
    connect(this, &QNetworkAccessManager::finished, this, &VK_Manager::got_postponed_posts);
}

void VK_Manager::get_albums() {
    QString url = "https://api.vk.com/method/photos.getAlbums?v=5.131"
                  "&access_token=" + m_access_token
                + "&owner_id=-" + m_group_id;
    get_url(url);
    connect(this, &QNetworkAccessManager::finished, this, &VK_Manager::got_albums);
}

void VK_Manager::post(int index, const QString& attachments, int date) {
    QString url = "https://api.vk.com/method/wall.post?v=5.131&from_group=1&signed=0"
                  "&primary_attachments_mode=carousel"
                  "&access_token=" + m_access_token
                + "&owner_id=-" + m_public_id
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
        int postponed_id = reply["response"].toObject()["post_id"].toInt();
        emit posted_successfully(index, date, postponed_id);
    });
}

void VK_Manager::get_poll(const QString& options, int end_date) {
    QUrl url("https://api.vk.com/method/polls.create");
    QUrlQuery params;
    params.addQueryItem("v", "5.199");
    params.addQueryItem("access_token", m_access_token);
    params.addQueryItem("owner_id", "-" + m_public_id);
    params.addQueryItem("question", "Тема пятничных постов");
    params.addQueryItem("is_anonymous", "1");
    params.addQueryItem("is_multiple", "1");
    params.addQueryItem("add_answers", options);
    params.addQueryItem("end_date", QString::number(end_date));
    url.setQuery(params);
    qDebug() << url;
    QNetworkReply* response = get(QNetworkRequest(url));
    connect(response, &QNetworkReply::finished, [this, response](){
        QJsonObject reply = reply_json(response);
        response->deleteLater();
        qDebug() << "reply" << reply;
        if (reply.contains("error")) {
            emit poll_post_failed(QJsonDocument(reply).toJson(QJsonDocument::Compact));
            return;
        }
        int poll_id = reply["response"].toObject()["id"].toInt();
        if (!poll_id) {
            emit poll_post_failed(QJsonDocument(reply).toJson(QJsonDocument::Compact));
            return;
        }
        emit poll_ready(poll_id);
    });
}

void VK_Manager::post(const QString& message, int poll_id, int date) {
    QString url = "https://api.vk.com/method/wall.post?v=5.199&from_group=1&signed=0"
                  "&access_token=" + m_access_token
                + "&owner_id=-" + m_public_id
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
                  "&access_token=" + m_access_token
                + "&owner_id=-" + m_group_id
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
            emit captcha_error(error["captcha_sid"].toString(), error["captcha_img"].toString());
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

void VK_Manager::got_captcha(QNetworkReply *response) {
    disconnect(this, &QNetworkAccessManager::finished, this, &VK_Manager::got_captcha);
    emit captcha_image_ready(image(response));
}

void VK_Manager::got_posts(QNetworkReply *response) {
    disconnect(this, &QNetworkAccessManager::finished, this, &VK_Manager::got_posts);
    auto reply = reply_json(response);
    auto array = reply["response"].toObject()["items"].toArray();
    int added = 0;
    int old = 0;
    int skipped = 0;
    int reposts = 0;
    int text_only = 0;
    for (const QJsonValueRef item : array) {
        auto current_item = item.toObject();
        if (current_item.contains("copy_history")) {
            ++reposts;
            continue;
        }
        if (!current_item.contains("attachments")) {
            ++text_only;
            continue;
        }
        int post_id = current_item["id"].toInt();
        int date = current_item["date"].toInt();
        auto attachments = current_item["attachments"].toArray();
        for (const QJsonValueRef attachment : attachments) {
            auto current_attachment = attachment.toObject();
            if (current_attachment["type"].toString() == "photo" && current_attachment["photo"].toObject()["owner_id"].toInt() == -m_group_id.toInt()) {
                QString photo_id = QString::number(current_attachment["photo"].toObject()["id"].toInt());
                if (!m_result.contains(photo_id)) {
                    QJsonObject info;
                    info["date"] = date;
                    info["post_id"] = post_id;
                    m_result[photo_id] = info;
                    ++added;
                } else {
                    ++old;
                }
            } else {
                ++skipped;
            }
        }
    }
    qDebug() << QString("Смещение %4. Пропущено: %1, репостов: %5, текстовых: %6, добавлено: %2, повторы: %3").arg(skipped).arg(added).arg(old).arg(m_offset).arg(reposts).arg(text_only);
    m_offset += 100;
    if (added || skipped)
        QTimer::singleShot(1000, this, &VK_Manager::get_posts);
    else
        emit posts_ready(m_result);
}

void VK_Manager::got_recent_posts(QNetworkReply *response) {
    disconnect(this, &QNetworkAccessManager::finished, this, &VK_Manager::got_recent_posts);
    QJsonObject result;
    QJsonArray result_array;
    auto reply = reply_json(response);
    if (!reply.size()) {
        qDebug() << "Не удалось получить данные о последних постах";
        return;
    }
    auto array = reply["response"].toObject()["items"].toArray();
    for (const QJsonValueRef item : array) {
        auto current_item = item.toObject();
        if (!current_item.contains("postponed_id"))
            continue;
        result_array.append(QJsonObject{
            {"postponed_id", current_item["postponed_id"].toInt()},
            {"date", current_item["date"].toInt()},
            {"post_id", current_item["id"].toInt()}
        });
    }
    result["result"] = result_array;
    emit recent_posts_ready(result);
}

void VK_Manager::got_postponed_posts(QNetworkReply *response) {
    disconnect(this, &QNetworkAccessManager::finished, this, &VK_Manager::got_postponed_posts);
    auto reply = reply_json(response);
    auto array = reply["response"].toObject()["items"].toArray();
    for (const QJsonValueRef item : array) {
        auto current_item = item.toObject();
        int post_id = -current_item["id"].toInt();
        int date = current_item["date"].toInt();
        auto attachments = current_item["attachments"].toArray();
        for (const QJsonValueRef attachment : attachments) {
            auto current_attachment = attachment.toObject();
            if (current_attachment["type"].toString() == "photo" && current_attachment["photo"].toObject()["owner_id"].toInt() == -m_group_id.toInt()) {
                QString photo_id = QString::number(current_attachment["photo"].toObject()["id"].toInt());
                if (!m_result.contains(photo_id)) {
                    QJsonObject info;
                    info["date"] = date;
                    info["post_id"] = post_id;
                    m_result[photo_id] = info;
                }
            }
        }
    }
    get_posts();
}
