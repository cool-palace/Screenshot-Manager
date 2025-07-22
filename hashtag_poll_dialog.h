#ifndef HASHTAGPOLLDIALOG_H
#define HASHTAGPOLLDIALOG_H

#include "hashtag_poll_button.h"
#include "hashtag_preview_db.h"
#include <QDialog>
#include <include/hashtag_info.h>
#include <ui_hashtag_dialog.h>

class HashtagPollDialog : public QDialog, public Ui_HashtagDialog
{
    Q_OBJECT
public:
    explicit HashtagPollDialog(const QMap<int, HashtagInfo>& hashtags, const QList<HashtagPreviewDB*>& selected, QWidget *parent = nullptr);
    HashtagInfo results() const { return m_result; }

public slots:
    void resizeEvent(QResizeEvent *event);
    void sort_hashtags();
    void lay_hashtags(QResizeEvent * event);

private:
    QMap<int, HashtagPollButton*> m_hashtags;
    HashtagInfo m_result;
    static int m_button_width;
    uint8_t m_current_tag_columns = 0;
};

#endif // HASHTAGPOLLDIALOG_H
