#ifndef HASHTAG_PREVIEW_DB_H
#define HASHTAG_PREVIEW_DB_H

#include <QWidget>
#include <include/hashtag_info.h>
#include <ui_hashtag_preview_db.h>

class HashtagPreviewDB : public QWidget, public Ui_HashtagPreviewDB
{
    Q_OBJECT

public:
    explicit HashtagPreviewDB(const HashtagInfo& info, QWidget *parent = nullptr);
    ~HashtagPreviewDB() {}

    void set_hashtag(const HashtagInfo& info);
    int id() const { return m_info.id; }
    QString name() const { return m_info.name; }
    QString option() const { return QString("\"%1 %2\"").arg(m_info.emoji, m_info.name); }
    QString message() const;
    bool description_edited() const { return m_info.description != leDescription->text(); }
    QString current_description() const { return leDescription->text(); }

private slots:
    void update();
    void reroll();
    void choose();
    void check_records();

signals:
    void dialog_start();
    void check_request();
    void changed();

private:
    HashtagInfo m_info;
};

#endif // HASHTAG_PREVIEW_DB_H
