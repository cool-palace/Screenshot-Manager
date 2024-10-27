#include "include\database.h"
#include <QDebug>

Database::Database(const QString& path) {
    db = QSqlDatabase::addDatabase("QSQLITE");
    db.setDatabaseName(path);
    if (!db.open()) {
        qDebug() << "Не удалось подключиться к базе данных";
        return;
    }
}

Database &Database::instance(const QString &path) {
    static Database instance(path);
    return instance;
}

Database::~Database() {
    qDebug() << "Закрытие базы данных";
    db.close();
}

void Database::add_file_data(QSqlQuery &query, int record_id, const QJsonArray &filenames, const QJsonArray &links, const QJsonArray &photo_ids) {
    for (int i = 0; i < filenames.size(); ++i) {
        query.prepare("INSERT INTO file_data (photo_id, record_id, filename, link) "
                      "VALUES (:photo_id, :record_id, :filename, :link)");
        query.bindValue(":photo_id", photo_ids[i].toInt());
        query.bindValue(":record_id", record_id);
        query.bindValue(":filename", filenames[i].toString());
        query.bindValue(":link", links[i].toString());
        query.exec();
    }
}

QList<int> Database::get_tag_ids(QSqlQuery &query, const QList<QPair<QString, int>> &tags) {
    QList<int> tag_ids;
    for (const auto& tag : tags) {
        query.prepare("SELECT id FROM hashtags WHERE tag = :tag");
        query.bindValue(":tag", tag.first);
        query.exec();
        if (query.next()) {
            tag_ids.append(query.value(0).toInt());
        } else {
            // Inserting new tag into database
            query.prepare("INSERT INTO hashtags (tag) VALUES (:tag)");
            query.bindValue(":tag", tag.first);
            query.exec();
            tag_ids.append(query.lastInsertId().toInt());
        }
    }
    return tag_ids;
}

int Database::add_series(QSqlQuery &query, const QString &series_name) {
    query.prepare("SELECT id FROM series WHERE name = :name");
    query.bindValue(":name", series_name);
    query.exec();
    if (query.next()) {
        return query.value(0).toInt();
    }
    // Inserting new series into database
    query.prepare("INSERT INTO series (name) VALUES (:name)");
    query.bindValue(":name", series_name);
    query.exec();
    return query.lastInsertId().toInt();
}

int Database::add_title(QSqlQuery &query, int series_id, int album_id, const QString& title_name, const QString& full_title) {
    query.prepare("SELECT id FROM titles WHERE name = :name AND series_id = :series_id");
    query.bindValue(":name", title_name);
    query.bindValue(":series_id", series_id);
    query.exec();
    if (query.next()) {
        return query.value(0).toInt();
    }
    // Inserting new title into database
    query.prepare("INSERT INTO titles (series_id, album_id, name, full_title) VALUES (:series_id, :album_id, :name, :full_title)");
    query.bindValue(":series_id", series_id);
    query.bindValue(":album_id", album_id);
    query.bindValue(":name", title_name);
    query.bindValue(":full_title", full_title);
    query.exec();
    return query.lastInsertId().toInt();
}

QPair<QString, QList<QPair<QString, int>>> Database::process_caption(const QString &caption) {
    QString text = caption;
    QList<QPair<QString, int>> tags;
    {   // Selecting main text
        QRegularExpression regex("(.*?)?([#&])(.*)?$");
        auto i = regex.globalMatch(caption);
        if (i.hasNext()) {
            auto match = i.peekNext();
            text = match.captured(1);
        } else text = "";
    }
    {   // Selecting tags
        QRegularExpression regex("[#&]([а-яё_123]+)");
        QRegularExpressionMatchIterator i = regex.globalMatch(caption);

        while (i.hasNext()) {
            QRegularExpressionMatch match = i.next();
            QString tag = match.captured(1);
            int type = caption[match.capturedStart()] == '#' ? 1 : 0;
            tags.append(qMakePair(tag, type));
        }
    }
    return qMakePair(text, tags);
}

void Database::add_record(QSqlQuery &query, const QJsonObject &record, int title_id) {
    auto caption_data = process_caption(record["caption"].toString());
    QString main_text = caption_data.first;
    QList<QPair<QString, int>> tags = caption_data.second;

    // Inserting new record into records table
    query.prepare("INSERT INTO records (caption, title_id, is_public) "
                  "VALUES (:caption, :title_id, :is_public)");
    query.bindValue(":caption", main_text);
    query.bindValue(":title_id", title_id);
    query.bindValue(":is_public", record["public"].toBool());
    query.exec();
    int record_id = query.lastInsertId().toInt();

    // Inserting new files into file_data table
    add_file_data(query, record_id, record["filenames"].toArray(), record["links"].toArray(), record["photo_ids"].toArray());

    // Inserting tag info into tag_record_map
    QList<int> tag_ids = get_tag_ids(query, tags);
    for (int i = 0; i < tag_ids.size(); ++i) {
        query.prepare("INSERT INTO tag_record_map (record_id, tag_id, type) "
                      "VALUES (:record_id, :tag_id, :type)");
        query.bindValue(":record_id", record_id);
        query.bindValue(":tag_id", tag_ids[i]);
        query.bindValue(":type", tags[i].second);
        query.exec();
    }
}

void Database::add_journal_data(QSqlQuery &query, const QJsonObject &object) {
    auto title = object["title"].toString();
    auto series = object["series"].toString();
    int album_id = object["album_id"].toInt();
    auto full_title = object["title_caption"].toString();

    // Adding series and title to database
    int series_id = add_series(query, series);
    int title_id = add_title(query, series_id, album_id, title, full_title);

    auto array = object["screens"].toArray();
    for (const auto& item : array) {
        add_record(query, item.toObject(), title_id);
    }
}

void Database::add_hashtags_data(QSqlQuery &query, const QJsonObject &hashtags_json) {
    for (const auto& tag : hashtags_json.keys()) {
        auto data = hashtags_json[tag].toObject();
        auto emoji = data["emoji"].toString();
        auto description = data["description"].toString();
        auto rank = data["rank"].toInt();

        query.prepare("INSERT OR IGNORE INTO hashtags (tag, emoji, description, rank) "
                      "VALUES (:tag, :emoji, :description, :rank)");
        query.bindValue(":tag", tag);
        query.bindValue(":emoji", emoji);
        query.bindValue(":description", description);
        query.bindValue(":rank", rank);
        query.exec();
    }
}

void Database::clear_main_tables(QSqlQuery &query) {
    query.exec("DROP TABLE IF EXISTS series");
    query.exec("UPDATE sqlite_sequence SET seq = 0 WHERE name = 'series'");

    query.exec("DROP TABLE IF EXISTS titles");
    query.exec("UPDATE sqlite_sequence SET seq = 0 WHERE name = 'titles'");

    query.exec("DROP TABLE IF EXISTS records");
    query.exec("UPDATE sqlite_sequence SET seq = 0 WHERE name = 'records'");

    query.exec("DROP TABLE IF EXISTS file_data");
    query.exec("UPDATE sqlite_sequence SET seq = 0 WHERE name = 'file_data'");

    query.exec("DROP TABLE IF EXISTS tag_record_map");
    query.exec("UPDATE sqlite_sequence SET seq = 0 WHERE name = 'tag_record_map'");
}

void Database::clear_hashtag_table(QSqlQuery &query) {
    query.exec("DROP TABLE IF EXISTS hashtags");
    query.exec("UPDATE sqlite_sequence SET seq = 0 WHERE name = 'hashtags'");
}

void Database::create_main_tables(QSqlQuery &query) {
    query.prepare("CREATE TABLE IF NOT EXISTS series ("
                      "id INTEGER PRIMARY KEY AUTOINCREMENT,"
                      "name VARCHAR(127))");
    if (!query.exec()) {
        qDebug() << "Не удалось создать таблицу 'series'" << query.lastError().text();
    }

    query.prepare("CREATE TABLE IF NOT EXISTS titles ("
                      "id INTEGER PRIMARY KEY AUTOINCREMENT,"
                      "series_id INTEGER,"
                      "album_id INTEGER,"
                      "name VARCHAR(127),"
                      "full_title VARCHAR(127),"
                      "FOREIGN KEY (series_id) REFERENCES series(id))");
    if (!query.exec()) {
        qDebug() << "Не удалось создать таблицу 'titles'" << query.lastError().text();
    }

    query.prepare("CREATE TABLE IF NOT EXISTS records ("
                      "id INTEGER PRIMARY KEY AUTOINCREMENT,"
                      "caption TEXT,"
                      "is_public BOOLEAN,"
                      "title_id INTEGER,"
                      "FOREIGN KEY (title_id) REFERENCES titles(id))");
    if (!query.exec()) {
        qDebug() << "Не удалось создать таблицу 'records'" << query.lastError().text();
    }

    query.prepare("CREATE TABLE IF NOT EXISTS file_data ("
                      "id INTEGER PRIMARY KEY AUTOINCREMENT,"
                      "photo_id INTEGER,"
                      "record_id INTEGER,"
                      "filename VARCHAR(255),"
                      "link TEXT,"
                      "FOREIGN KEY (record_id) REFERENCES records(id))");
    if (!query.exec()) {
        qDebug() << "Не удалось создать таблицу 'file_data'" << query.lastError().text();
    }

    query.prepare("CREATE TABLE IF NOT EXISTS tag_record_map ("
                      "id INTEGER PRIMARY KEY AUTOINCREMENT,"
                      "record_id INTEGER,"
                      "tag_id INTEGER,"
                      "type INTEGER,"
                      "FOREIGN KEY (record_id) REFERENCES records(id),"
                      "FOREIGN KEY (tag_id) REFERENCES hashtags(id))");
    if (!query.exec()) {
        qDebug() << "Не удалось создать таблицу 'tag_record_map'" << query.lastError().text();
    }
}

void Database::reset_main_tables(QSqlQuery &query) {
    clear_main_tables(query);
    create_main_tables(query);
}

void Database::reset_hashtag_table(QSqlQuery &query) {
    query.prepare("SELECT COUNT(*) FROM sqlite_master WHERE type='table' AND name='hashtags'");
    if (!query.exec() || !query.next()) {
        qDebug() << "Ошибка выполнения запроса на проверку таблицы:" << query.lastError();
        return;
    }
    bool table_exists = query.value(0).toInt() > 0;
    qDebug() << table_exists;
    if (table_exists) {
        clear_hashtag_table(query);
    }
    create_hashtag_table(query);
}

void Database::create_hashtag_table(QSqlQuery &query) {
    query.prepare("CREATE TABLE IF NOT EXISTS hashtags ("
                      "id INTEGER PRIMARY KEY AUTOINCREMENT,"
                      "tag VARCHAR(31) UNIQUE,"
                      "emoji VARCHAR(1),"
                      "description VARCHAR(255),"
                      "rank INTEGER)");
    if (!query.exec()) {
        qDebug() << "Не удалось создать таблицу 'hashtags'" << query.lastError().text();
    }
}

void Database::create_record_logs_table(QSqlQuery &query) {
    query.prepare("CREATE TABLE IF NOT EXISTS record_logs ("
                      "id INTEGER PRIMARY KEY AUTOINCREMENT,"
                      "photo_id INTEGER,"
                      "date TIMESTAMP)");
    if (!query.exec()) {
        qDebug() << "Не удалось создать таблицу 'record_logs'" << query.lastError().text();
    }
}

void Database::create_poll_logs_table(QSqlQuery &query) {
    query.prepare("CREATE TABLE IF NOT EXISTS poll_logs ("
                      "id INTEGER PRIMARY KEY AUTOINCREMENT,"
                      "tag VARCHAR(31),"
                      "date TIMESTAMP)");
    if (!query.exec()) {
        qDebug() << "Не удалось создать таблицу 'poll_logs'" << query.lastError().text();
    }
}
