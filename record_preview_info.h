#ifndef RECORDPREVIEWINFO_H
#define RECORDPREVIEWINFO_H
#include <QDateTime>
#include <QSqlQuery>
#include <include/recordpreview.h>
#include <include/database.h>

struct RecordPreviewInfo {
    RecordPreviewInfo() = default;
    RecordPreviewInfo(const QSqlQuery& query) {
        id = query.value("record_id").toInt();
        date = query.value("date").toDateTime();
        quote = query.value("quote").toString();
        hashtags = query.value("tag_string").toString();
        title = query.value("title_name").toString();
        links = query.value("links").toString().split('|');
    }
    int id;
    QDateTime date;
    QString quote;
    QString hashtags;
    QString title;
    QStringList links;
};

#endif // RECORDPREVIEWINFO_H
