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
        photo_ids = query.value("photo_ids").toString().split('|');
    }
    const QString attachments() const {
        QString prefix = VK_Manager::instance().prefix();
        return prefix + photo_ids.join("," + prefix);
    }
    int id;
    QDateTime date;
    QString quote;
    QString hashtags;
    QString title;
    QStringList links;
    QStringList photo_ids;
};

class RecordPreviewBase : public QWidget {
public:
    RecordPreviewBase(const RecordPreviewInfo& record, const QDateTime& time, QWidget *parent = nullptr)
        : QWidget(parent), m_info(std::move(record)), m_time(time) {   }

public slots:
    void post() const {
        VK_Manager::instance().post(m_info.id, m_info.attachments(), m_time.toSecsSinceEpoch());
    }
    QMap<int, QDateTime> logs_data() const {
        QMap<int, QDateTime> result;
        for (const QString& id : m_info.photo_ids)
            result.insert(id.toInt(), m_time);
        return result;
    }
    QMap<int, QPair<QDateTime, int>> logs_data(int post_id) const {
        QMap<int, QPair<QDateTime, int>> result;
        for (const QString& id : m_info.photo_ids)
            result.insert(id.toInt(), qMakePair(m_time, -post_id));
        return result;
    }

protected:
    RecordPreviewInfo m_info;
    QList<QSharedPointer<RecordFrame>> m_images;
    QDateTime m_time;
};

#endif // RECORDPREVIEWINFO_H
