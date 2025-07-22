#ifndef HASHTAG_DIALOG_H
#define HASHTAG_DIALOG_H

#include <QDialog>
#include <include/query_filters.h>
#include <include/hashtag_info.h>
#include <hashtag_button_db.h>
#include <ui_hashtag_dialog.h>

class HashtagDialog : public QDialog, public Ui_HashtagDialog
{
    Q_OBJECT

public:
    explicit HashtagDialog(HashtagsFilter& filters, const QMap<int, HashtagInfo>& hashtags, QWidget *parent = nullptr);
    ~HashtagDialog() {}
    HashtagsFilter old_results() const { return m_prev_filters; };

signals:
     void update_count();

public slots:
    void resizeEvent(QResizeEvent *event);
    void filter_event(const HashtagFilterType, const int);

private slots:
    void set_filter_buttons();
    void get_hashtag_ranks();
    void sort_hashtags();
    void lay_hashtags(QResizeEvent * event);

private:
    QMap<int, HashtagButtonDB*> m_hashtags;
    HashtagsFilter& m_filters;
    HashtagsFilter m_prev_filters;
    uint8_t m_current_tag_columns = 0;
    std::set<int> m_checked_ids;
    static QPair<int, int> m_rank_range;
    static int m_button_width;
};

#endif // HASHTAG_DIALOG_H
