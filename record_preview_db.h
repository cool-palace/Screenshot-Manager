#ifndef RECORD_PREVIEW_DB_H
#define RECORD_PREVIEW_DB_H
#include <QDateTime>
#include <QSqlQuery>
#include <QWidget>
#include <include/recordpreview.h>
#include <include/query_filters.h>
#include "record_preview_info.h"
#include <ui_record_preview_db.h>

class RecordPreviewDB : public QWidget, public Ui_RecordPreviewDB
{
    Q_OBJECT

public:
    explicit RecordPreviewDB(const RecordPreviewInfo&, const QDateTime&, QWidget *parent = nullptr);
    ~RecordPreviewDB();

public slots:
    void set_index(int);
    void set_record(const RecordPreviewInfo&);
    void update_log_info();
    void set_next(RecordPreviewDB* next) { m_next = next; }
    void set_prev(RecordPreviewDB* prev) { m_prev = prev; }

private slots:
    void switch_up();
    void switch_down();
    void search();
    void set_time();
    void reroll();
    void input_number();
    void update();
    void update_time();
    void update_images();
    void update_text();

private:
    RecordPreviewInfo m_info;
    QList<QSharedPointer<RecordFrame>> m_images;
    QDateTime m_time;
    RecordPreviewDB * m_prev = nullptr;
    RecordPreviewDB * m_next = nullptr;
};

#endif // RECORD_PREVIEW_DB_H
