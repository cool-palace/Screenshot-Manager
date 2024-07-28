#ifndef JOURNAL_READING_H
#define JOURNAL_READING_H
#include <QObject>
#include "include\abstract_mode.h"

class JournalReading : public AbstractOperationMode
{
    Q_OBJECT
public:
    explicit JournalReading(MainWindow *parent, bool all = false);
    virtual ~JournalReading();

private:
    bool record_edited = false;
    QStringList current_hashtags;
    QSet<QPair<int, int>> edited_ranges;
    QMap<int, QString> captions_for_ids;

public slots:
    virtual void start() override;
    virtual void keyPressEvent(QKeyEvent*) override;
    virtual void keyReleaseEvent(QKeyEvent*) override;
    virtual void closeEvent(QCloseEvent*) override;
    bool unsaved_changes() const { return !edited_ranges.empty(); }

protected slots:
    virtual void slider_change(int) override;
    virtual void skip_button() override {}
    virtual void add_button() override;
    virtual void back_button() override;
    virtual void ok_button() override;
    virtual void set_view(View) override;
    virtual void lay_previews(int page = 1) override;
    virtual void show_status() override;
    virtual void create_hashtag_button(const QString&) override;
    void hashtag_event(const QChar&, const QString&);
    virtual bool data_ready() override;
    void set_enabled(bool);

private:
    bool open_title_journal(bool all = false);
    void read_title_journal(const QJsonObject&);
    void save_title_journal(int, int);
    void display(int);
    void set_edited();
    bool update_quote_file(int, int);
    void update_record();
    void save_changes();
    void show_public(bool);
    void show_private(bool);
    void add_caption(const QString& captcha_sid = "", const QString& captcha_key = "");
    void caption_success();
    void captcha_handling(const QString&);
    QString preprocessed(const QString&) const;
    void load_hashtag_info();
    void update_hashtag_info();
    void update_current_hashtags();
    void highlight_current_hashtags(bool);
    void recalculate_hashtags(bool);
    void export_captions_by_ids();
    void export_info_by_ids();
};

#endif // JOURNAL_READING_H
