#ifndef RELEASE_PREPARATION_DB_H
#define RELEASE_PREPARATION_DB_H
#include <include/query_filters.h>
#include <include/vk_manager.h>
#include <include/hashtag_button.h>
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
    void series_dialog();

    bool open_database();
    void generate_button();
    void generate_release();
    void post_button();
    void posting_success(int, int);
    void posting_fail(int, const QString&);

protected:
    std::set<int> m_series;
    QStringList m_hashtags;
    QueryFilters m_filters;
};

#endif // RELEASE_PREPARATION_DB_H
