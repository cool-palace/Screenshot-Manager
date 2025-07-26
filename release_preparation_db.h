#ifndef RELEASE_PREPARATION_DB_H
#define RELEASE_PREPARATION_DB_H
#include "record_preview_db.h"
#include "title_group.h"
#include <include/query_filters.h>
#include <include/vk_manager.h>
#include <include/series_info.h>
#include <include/hashtag_info.h>
#include <ui_release_preparation_db.h>
#include <set>

class ReleasePreparationDB : public QWidget, public Ui_ReleasePreparationDB
{
    Q_OBJECT

public:
    explicit ReleasePreparationDB(QWidget *parent = nullptr);

public slots:
    void start();

private slots:
    void set_enabled(bool enable);

    void publicity_filter_changed();
    void quantity_filter_changed();
    void last_used_filter_changed();
    void series_filter_changed();
    void hashtags_filter_changed();
    void text_filter_changed();
    void text_filter_reset();
    void update_results();
    void get_series_info();
    void series_dialog();
    void hashtag_dialog();
    void update_hashtag_count();

    void generate_button();
    void post_button();

    QString hashtag_filters();

protected:
    int m_series_size;
    QList<SeriesInfo> m_series_info;
    QMap<int, HashtagInfo> m_hashtag_info;
    QList<RecordPreviewDB*> m_selected_records;
    QMutex m_status_mutex;
    int m_post_counter = 0;
};

#endif // RELEASE_PREPARATION_DB_H
