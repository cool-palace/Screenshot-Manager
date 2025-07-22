#ifndef HASHTAGPOLLBUTTON_H
#define HASHTAGPOLLBUTTON_H

#include <QPushButton>
#include <QObject>
#include <include/hashtag_info.h>
#include <include/query_filters.h>

class HashtagPollButton : public QPushButton
{
    Q_OBJECT
public:
    explicit HashtagPollButton(const HashtagInfo& info, QWidget *parent = nullptr);
    void mousePressEvent(QMouseEvent*) override;
    void show_count();
    int id() const { return m_info.id; }
    int count() const { return m_info.count; }
    QString name() const { return m_info.name; }
    QDateTime date() const { return m_info.date; }
signals:
    void selected(const HashtagInfo&);
private:
    HashtagInfo m_info;
};

#endif // HASHTAGPOLLBUTTON_H
