#ifndef POSTPROGRESSDIALOG_H
#define POSTPROGRESSDIALOG_H
#include <QDialog>
#include <QTextEdit>
#include "hashtag_preview_db.h"
#include "record_poll_preview.h"
#include "record_preview_db.h"

class PostingProgressDialog : public QDialog {
    Q_OBJECT

private:
    PostingProgressDialog(QWidget *parent = nullptr);

public:
    PostingProgressDialog(const QList<RecordPreviewDB*> &records, QWidget *parent = nullptr);
    PostingProgressDialog(const QList<HashtagPreviewDB*>& hashtags, QPair<QDateTime, QDateTime>&& poll_time, const QList<RecordPollPreview*> &records, QWidget *parent = nullptr);
    void start_posting();

signals:
    void finished_all();

private slots:
    void handle_success(int index, int date, int post_id);
    void handle_failure(int index, const QString &error);
    void post_next();
    void handle_poll_success();
    void handle_poll_failure(const QString &error);
    void get_poll();
    void post_poll(int id);
    void update_record_logs();
    void update_poll_logs();
    void update_hashtags();
    void update_recent_record_logs();
    void handle_recent_posts(const QJsonObject& json);

private:
    QString poll_options() const;
    QString poll_message() const;

private:
    QList<RecordPreviewBase*> m_records;
    QList<HashtagPreviewDB*> m_hashtags;
    QPair<QDateTime, QDateTime> m_poll_times;
    QList<int> m_post_ids;      // Можно внести данные в RecordPreviewBase
    int m_current = 0;
    int m_success_count = 0;
    int m_fail_count = 0;

    QProgressBar *pbProgress;
    QLabel *lblStatus;
    QTextEdit *teLog;
};

#endif // POSTPROGRESSDIALOG_H
