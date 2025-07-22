#ifndef RECORD_POLL_PREVIEW_H
#define RECORD_POLL_PREVIEW_H
#include "record_preview_info.h"
#include <include/recordpreview.h>
#include <include/hashtag_info.h>
#include <ui_record_poll_preview.h>

struct HashtagPairInfo {
    HashtagPairInfo(const QList<int>& ids, QPair<HashtagInfo, HashtagInfo>&& tags) :
        record_ids(ids), tags(std::move(tags)) {};
    HashtagPairInfo(const HashtagPairInfo&) = default;
    QList<int> record_ids;
    QPair<HashtagInfo, HashtagInfo> tags;
};

class RecordPollPreview : public QWidget, public Ui_RecordPollPreview
{
    Q_OBJECT

public:
    explicit RecordPollPreview(const RecordPreviewInfo&, const HashtagPairInfo&, const QDateTime&, QWidget *parent = nullptr);
    ~RecordPollPreview();

public slots:
    void set_index(int);
    void set_record(const RecordPreviewInfo&);
    void set_hashtags(const HashtagPairInfo&);
    void update_log_info();
    void set_next(RecordPollPreview* next) { m_next = next; }
    void set_prev(RecordPollPreview* prev) { m_prev = prev; }
    void reset_spinbox();

private slots:
    void switch_up();
    void switch_down();
    void search();
    void set_time();
    void spinbox_changed(int);
    void update();
    void update_time();
    void update_images();
    void update_text();
    void update_tags();

private:
    RecordPreviewInfo m_info;
    HashtagPairInfo m_tag_info;
    QList<QSharedPointer<RecordFrame>> m_images;
    QDateTime m_time;
    RecordPollPreview * m_prev = nullptr;
    RecordPollPreview * m_next = nullptr;
};

#endif // RECORD_POLL_PREVIEW_H
