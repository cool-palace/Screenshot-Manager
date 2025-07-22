#ifndef HASHTAG_BUTTON_DB_H
#define HASHTAG_BUTTON_DB_H
#include <QMenu>
#include <QPushButton>
#include <include/hashtag_info.h>
#include <include/query_filters.h>

enum HashtagFilterType {
    INCLUDE = 0,
    EXCLUDE = 1,
    HASHTAG = 1 << 1,
    AMPTAG = 1 << 2,
    HASHTAG_EXCLUDE = HASHTAG | EXCLUDE,
    AMPTAG_EXCLUDE = AMPTAG | EXCLUDE,
    ANY_TAG = HASHTAG | AMPTAG,
    ANY_TAG_EXCLUDE = ANY_TAG | EXCLUDE
};

enum class HashtagActions {
    NO_ACTION = -1,
    INCLUDE_ALL = 0,
    INCLUDE_HASH,
    INCLUDE_AMP,
    EXCLUDE_ALL,
    EXCLUDE_HASH,
    EXCLUDE_AMP,
};

class HashtagButtonDB : public QPushButton
{
    Q_OBJECT
public:
    explicit HashtagButtonDB(const HashtagInfo& info, QWidget *parent = nullptr);
    void create_actions();
    void mousePressEvent(QMouseEvent*) override;
    void highlight(const HashtagActions, bool);
    void set_count(int count);
    void show_count();
    int id() const { return m_info.id; }
    int count() const { return m_info.count; }
    int rank() const { return m_info.rank; }
    QString name() const { return m_info.name; }
    void trigger(HashtagActions action);
    void set_filter(const bool include, const HashtagFilter filter);
signals:
    void filterEvent(const HashtagFilterType, const int id);
private slots:
    void select_action(HashtagActions);
private:
    HashtagInfo m_info;
    QMenu m_menu;
    HashtagActions m_selected_action = HashtagActions::NO_ACTION;
};

#endif // HASHTAG_BUTTON_DB_H
