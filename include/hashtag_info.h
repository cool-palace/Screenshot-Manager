#ifndef HASHTAG_INFO_H
#define HASHTAG_INFO_H
#include <QDateTime>
#include <include/database.h>
#include <QString>

struct HashtagInfo {
    HashtagInfo() = default;
    HashtagInfo(int id, const QString& name, int rank, int count = 0) : id(id), name(name), rank(rank), count(count) {};
    HashtagInfo(int id, const QString& name, const QString& emoji, const QString& description) : id(id), name(name), emoji(emoji), description(description) {};
    HashtagInfo(const QSqlQuery& query) {
        id = query.value("id").toInt();
        count = query.value("count").toInt();
        name = query.value("name").toString();
        emoji = query.value("emoji").toString();
        description = query.value("description").toString();
        date = query.value("date").toDateTime();
    }
    int id;
    QString name;
    int rank;
    int count;
    QString emoji;
    QString description;
    QDateTime date;
};
#endif // HASHTAG_INFO_H
