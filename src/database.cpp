#include "include\database.h"
#include <QDebug>

Database::Database() { }

Database &Database::instance() {
    static Database instance;
    return instance;
}

Database::~Database() {
    db.close();
}

void Database::init(const QString &path) {
    db = QSqlDatabase::addDatabase("QSQLITE");
    db.setDatabaseName(path);
    if (!db.open()) {
        qDebug() << "Не удалось подключиться к базе данных";
        return;
    }
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

void Database::select_records(QSqlQuery &query, const QueryFilters &filters, bool random, int limit, int offset) {
    QString query_str = QString("WITH filtered_records AS ( %1 ),"
                                "   tags AS ("
                                "       SELECT trm.record_id,"
                                "           GROUP_CONCAT("
                                "               CASE "
                                "                   WHEN trm.type = 1 THEN '#' || h.tag"
                                "                   ELSE '&' || h.tag    "
                                "               END,"
                                "               ' '"
                                "           ) AS tag_string"
                                "       FROM tag_record_map trm"
                                "       JOIN hashtags h ON trm.tag_id = h.id"
                                "       GROUP BY trm.record_id),"
                                "   links AS ("
                                "       SELECT fd.record_id,"
                                "           GROUP_CONCAT(fd.link, '|') AS links"
                                "       FROM file_data fd"
                                "       GROUP BY fd.record_id)"
                                "   SELECT fr.id AS record_id,"
                                "       fr.caption AS quote,"
                                "       tags.tag_string,"
                                "       links.links,"
                                "       fr.date,"
                                "       t.name AS title_name"
                                "   FROM filtered_records fr"
                                "   LEFT JOIN tags ON tags.record_id = fr.id"
                                "   LEFT JOIN links ON links.record_id = fr.id"
                                "   LEFT JOIN titles t ON fr.title_id = t.id ").arg(select_query(filters));
    // Порядок
    query_str += QString("ORDER BY %2 ").arg(random ? "RANDOM()" : "fr.id");
    // Ограничиваем количество
    if (limit > 0)
        query_str += QString("LIMIT %1 ").arg(limit);
    if (offset > 0)
        query_str += QString("OFFSET %1 ").arg(offset);

    query.prepare(query_str);
    if (!query.exec()) {
        qDebug() << "Не выполнить поиск записей по фильтрам" << query.lastError().text();
    }
}

int Database::count_records(const QueryFilters &filters) {
    QSqlQuery query;
    QString query_str = QString("SELECT COUNT(*) as count FROM ( %1 )").arg(select_query(filters));
    query.prepare(query_str);
    if (!query.exec()) {
        qDebug() << "Не выполнить получить количество результатов по фильтрам" << query.lastError().text();
        return 0;
    }
    if (query.next())
        return query.value("count").toInt();
    return 0;
}

int Database::count_records() {
    QSqlQuery query;
    query.prepare("SELECT COUNT(*) AS count FROM records r");
    if (!query.exec()) {
        qDebug() << "Не выполнить получить общее количество записей" << query.lastError().text();
        return 0;
    }
    if (query.next())
        return query.value("count").toInt();
    return 0;
}

void Database::count_series(QSqlQuery &query) {
    query.prepare("SELECT COUNT(*) AS count FROM series s");
    if (!query.exec()) {
        qDebug() << "Не выполнить получить количество сериалов" << query.lastError().text();
    }
}

void Database::select_excluded_series_ids(QSqlQuery &query, const QDate &date) {
    query.prepare(select_series_query(date));
    if (!query.exec()) {
        qDebug() << "Не выполнить получить список сериалов по дате" << query.lastError().text();
    }
}

void Database::select_series_info(QSqlQuery &query) {
    query.prepare("SELECT s.id AS id, s.name AS series_name, COALESCE(record_counts.record_count, 0) AS record_count, "
                  "t.name || '\\' || fd.filename AS filepath "
                  "FROM series s "
                  "LEFT JOIN ( "
                  "    SELECT t.series_id, COUNT(r.id) AS record_count "
                  "    FROM titles t "
                  "    LEFT JOIN records r ON t.id = r.title_id "
                  "    GROUP BY t.series_id "
                  ") AS record_counts ON s.id = record_counts.series_id "
                  "LEFT JOIN titles t ON s.id = t.series_id "
                  "LEFT JOIN records r ON t.id = r.title_id "
                  "LEFT JOIN file_data fd ON r.id = fd.record_id "
                  "WHERE fd.id = ( "
                  "    SELECT MIN(fd_inner.id) "
                  "    FROM file_data fd_inner "
                  "    JOIN records r_inner ON fd_inner.record_id = r_inner.id "
                  "    JOIN titles t_inner ON r_inner.title_id = t_inner.id "
                  "    WHERE t_inner.series_id = s.id "
                  ")");
    if (!query.exec()) {
        qDebug() << "Не выполнить получить информацию по сериалам" << query.lastError().text();
    }
}

void Database::select_hashtag_info(QSqlQuery &query) {
    query.prepare("SELECT h.id, h.tag AS name, h.rank FROM hashtags h");
    if (!query.exec()) {
        qDebug() << "Не выполнить получить информацию по хэштегам" << query.lastError().text();
    }
}

void Database::count_hashtags(QSqlQuery &query, const QueryFilters &filters) {
    QString query_str = QString("WITH filtered_records AS ( %1 ) "
                                "SELECT h.id, COUNT(DISTINCT fr.id) AS count "
                                "FROM hashtags h "
                                "LEFT JOIN tag_record_map trm ON trm.tag_id = h.id "
                                "LEFT JOIN filtered_records fr ON fr.id = trm.record_id "
                                "GROUP BY h.id "
                                "ORDER BY h.id; ").arg(select_query(filters));
    query.prepare(query_str);
    if (!query.exec()) {
        qDebug() << "Не выполнить получить количество хэштегов" << query.lastError().text();
    }
}

void Database::select_hashtag_ranks(QSqlQuery &query) {
    query.prepare("SELECT MIN(h.rank) AS min_rank, MAX(h.rank) AS max_rank FROM hashtags h");
    if (!query.exec()) {
        qDebug() << "Не выполнить получить уровни хэштегов" << query.lastError().text();
    }
}

void Database::select_record_by_id(QSqlQuery &query, int id) {
    query.prepare(" WITH tags AS ("
                  "     SELECT trm.record_id,"
                  "         GROUP_CONCAT("
                  "             CASE "
                  "                 WHEN trm.type = 1 THEN '#' || h.tag"
                  "                 ELSE '&' || h.tag    "
                  "             END, ' ') AS tag_string"
                  "     FROM tag_record_map trm"
                  "     JOIN hashtags h ON trm.tag_id = h.id"
                  "     GROUP BY trm.record_id),"
                  " links AS ("
                  "     SELECT fd.record_id,"
                  "            GROUP_CONCAT(fd.link, '|') AS links"
                  "     FROM file_data fd"
                  "     GROUP BY fd.record_id),"
                  " latest_date AS ("
                  "     SELECT r.id AS record_id, MAX(rl.date) AS date"
                  "     FROM records r"
                  "     JOIN file_data fd ON fd.record_id = r.id"
                  "     JOIN record_logs rl ON rl.photo_id = fd.photo_id"
                  "     GROUP BY r.id)"
                  " SELECT r.id AS record_id,"
                  "     r.caption AS quote,"
                  "     tags.tag_string,"
                  "     links.links,"
                  "     latest_date.date,"
                  "     t.name AS title_name"
                  " FROM records r"
                  " LEFT JOIN latest_date ON latest_date.record_id = r.id"
                  " LEFT JOIN tags ON tags.record_id = r.id"
                  " LEFT JOIN links ON links.record_id = r.id"
                  " LEFT JOIN titles t ON r.title_id = t.id"
                  " WHERE r.id = :id");
    query.bindValue(":id", id);
    if (!query.exec()) {
        qDebug() << "Не выполнить получить запись" << id << query.lastError().text();
    }
}

QString Database::select_query(const QueryFilters &filters) {
    static const QString base = "SELECT r.id, r.caption, rl.date, t.id AS title_id "
                                "FROM records r "
                                "LEFT JOIN titles t ON r.title_id = t.id "
                                "LEFT JOIN series s ON t.series_id = s.id "
                                "LEFT JOIN file_data fd ON r.id = fd.record_id "
                                "LEFT JOIN record_logs rl ON fd.photo_id = rl.photo_id "
                                "LEFT JOIN tag_record_map trm ON r.id = trm.record_id "
                                "LEFT JOIN hashtags h ON trm.tag_id = h.id "
                                "WHERE 1=1 \n";
    QString query_str;
    const bool hashtags_enabled = filters.hashtags.enabled && (filters.hashtags.included.size() || filters.hashtags.excluded.size());
    // Если заданы фильтры по хэштегам
    if (hashtags_enabled) {
        // Создаём временные таблицы к хэштегам
        QStringList temp_values;
        QStringList excluded_values;

        for (const HashtagFilter& tag : filters.hashtags.included)
            temp_values << tag.to_sql();

        for (const HashtagFilter& tag : filters.hashtags.excluded)
            excluded_values << tag.to_sql();

        QStringList withParts;

        if (!temp_values.isEmpty())
            withParts << QString("temp_tags(id, type) AS (VALUES %1 )").arg(temp_values.join(", "));

        if (!excluded_values.isEmpty())
            withParts << QString("excluded_tags(id, type) AS (VALUES %1)").arg(excluded_values.join(", "));

        query_str = QString("WITH " + withParts.join(",\n") + "\n");
    }
    // Базовый запрос
    query_str += base;
    // Фильтр по публичности
    if (filters.publicity.enabled) {
        query_str += QString("AND r.is_public = %1 \n").arg(filters.publicity.hidden ? "false" : "true");
    }
    // Фильтр по источникам
    if (filters.series.enabled) {
        if (filters.series.last_used) {
            query_str += QString("AND s.id NOT IN (%1) ").arg(select_series_query(filters.series.date));
        } else {
            QString list;
            bool included = filters.series.included.size() < filters.series.excluded.size();
            const std::set<int>& series = included ? filters.series.included : filters.series.excluded;
            for (const auto& id : series) {
                if (!list.isEmpty())
                    list += ", ";
                list += QString("%1").arg(id);
            }
            query_str += QString("AND s.id %1 IN (%2) ").arg(included ? "" : "NOT").arg(list);
        }
    }
    // Фильтр по дате
    if (filters.last_used.enabled) {
        query_str += QString("AND (rl.date IS NULL OR rl.date < DATE('%1')) \n").arg(filters.last_used.date.toString(Qt::ISODate));
    }
    // Фильтр по хэштегам
    if (hashtags_enabled) {
        if (filters.hashtags.included.size())
            query_str += "AND r.id IN ( "
                            "SELECT trm.record_id "
                            "FROM tag_record_map trm "
                            "JOIN temp_tags t ON trm.tag_id = t.id AND (t.type IS NULL OR trm.type = t.type) "
                            "GROUP BY trm.record_id "
                            "HAVING COUNT(DISTINCT t.id) = (SELECT COUNT(*) FROM temp_tags)) \n";
        if (filters.hashtags.excluded.size())
            query_str += "AND r.id NOT IN ( "
                            "SELECT trm.record_id "
                            "FROM tag_record_map trm "
                            "JOIN excluded_tags t ON trm.tag_id = t.id AND (t.type IS NULL OR trm.type = t.type) "
                            "GROUP BY trm.record_id) \n";
    }
    // Фильтр по тексту
    if (filters.text.enabled) {
        QString lower = filters.text.text;
        QString upper = lower;

        upper[0] = upper[0].toUpper();
        lower[0] = lower[0].toLower();

        // Если заглавная и строчная версии одинаковы — одно условие
        if (lower == upper)
            query_str += QString("AND r.caption LIKE '%%1%' \n").arg(lower);
        else
            query_str += QString("AND (r.caption LIKE '%%1%' OR r.caption LIKE '%%2%') \n").arg(lower, upper);
    }
    // Собираем по r.id
    query_str += "GROUP BY r.id ";
    // Фильтр по количеству кадров
    if (filters.quantity.enabled) {
        QChar sign = filters.quantity.multiple ? '>' : '=';
        query_str += QString("HAVING COUNT(DISTINCT fd.id) %1 1 ").arg(sign);
    }
    qDebug() << query_str;
    return query_str;
}

QString Database::select_series_query(const QDate &date) {
    return QString("SELECT DISTINCT s_inner.id "
                        "FROM series s_inner "
                        "JOIN titles t_inner ON s_inner.id = t_inner.series_id "
                        "JOIN records r_inner ON t_inner.id = r_inner.title_id "
                        "JOIN file_data fd_inner ON r_inner.id = fd_inner.record_id "
                        "JOIN record_logs rl_inner ON fd_inner.photo_id = rl_inner.photo_id "
                   "WHERE rl_inner.date >= DATE('%1') \n").arg(date.toString(Qt::ISODate));
}
