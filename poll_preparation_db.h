#ifndef POLL_PREPARATION_DB_H
#define POLL_PREPARATION_DB_H
#include "record_preview_db.h"
#include "record_poll_preview.h"
#include "hashtag_preview_db.h"
#include <include/query_filters.h>
#include <include/hashtag_info.h>
#include <ui_poll_preparation_db.h>

class PollPreparationDB : public QWidget, public Ui_PollPreparationDB
{
    Q_OBJECT

public:
    explicit PollPreparationDB(QWidget *parent = nullptr);

public slots:
    void start();

private slots:
    void set_enabled(bool enable);

    void publicity_filter_changed();
    void last_used_filter_changed();
    void poll_filter_changed();
    void update_results();
    void hashtag_dialog();
    void update_hashtag_count();
    void prepare_tag_map();
    void set_cycle(int);
    void display_matrix(const QVector<QVector<int>>& matrix);

    void generate_hashtags();
    void tag_pairing_analysis();
    void generate_posts();
    void generate_release();
    void post_button();
    void posting_success(int, int);
    void posting_fail(int, const QString&);

    QVector<QVector<int>> matrix(QSqlQuery& query, const QList<int>& tag_ids);
    QList<HashtagPairInfo> tag_pairs(const QVector<int>& cycle) const;
    QString hashtag_filters();

protected:
    QMap<int, HashtagInfo> m_hashtag_info;
    QList<HashtagPreviewDB*> m_selected_hashtags;
    QList<RecordPollPreview*> m_selected_records;
    QList<QVector<int>> m_hamiltonian_cycles;
    QHash<QSet<int>, QList<int>> m_tags_records_map;
};

#endif // POLL_PREPARATION_DB_H
