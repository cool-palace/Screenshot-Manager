#ifndef MAINWINDOW_H
#define MAINWINDOW_H

#include <QMainWindow>
#include <QNetworkAccessManager>
#include <QNetworkReply>
#include <QDir>
#include <QFileDialog>
#include <QLabel>
#include <QJsonArray>
#include <QJsonDocument>
#include <QJsonObject>
#include <QKeyEvent>
#include <QInputDialog>
#include <QRegularExpression>
#include <QMessageBox>
#include <QProgressDialog>
#include "vk_manager.h"
#include "hashtag_button.h"
#include "record.h"

namespace Ui {
class MainWindow;
}

class MainWindow : public QMainWindow
{
    Q_OBJECT
    enum Mode {
        IDLE,
        JOURNAL_CREATION,
        JOURNAL_READING,
        TEXT_READING,
        RELEASE_PREPARATION,
        DESCRIPTION_READING
    };
    enum View {
        MAIN,
        LIST,
        GALLERY,
        PREVIEW,
        TITLES
    };
    enum Directories {
        JOURNALS,
        SCREENSHOTS,
        QUOTES,
        SUBS,
        LOGS,
        POLL_LOGS
    };

public:
    explicit MainWindow(QWidget *parent = nullptr);
    ~MainWindow() override;

protected:
    void closeEvent(QCloseEvent*) override;

public slots:
    // Network slots
    void set_albums(const QMap<QString, int>&);
    void set_photo_ids(const QVector<int>&, const QStringList&);
    void set_loaded_image(const QImage&);
    void posting_success(int, int);
    void posting_fail(int, const QString&);
    void post_poll(int);
    void poll_posting_success();
    void poll_posting_fail(const QString&);
    void caption_success();
    void captcha_handling(const QString&);
    // Launching modes
    void journal_creation();
    void journal_reading();
    void journal_reading_all();
    void text_reading();
    void descriptions_reading();
    void release_preparation();
    void poll_preparation();
    // UI elements
    void show_public(bool);
    void show_private(bool);
    void slider_change(int);
    // Control buttons
    void add_hashtag();
    void load_subs();
    void generate_button();
    void generate_release();
    void generate_poll();
    void check_logs();
    void post_button();
    void skip_button();
    void add_button();
    void back_button();
    void ok_button();

    void hashtag_event(const QChar&, const QString&);
    void filter_event(bool);                        // Public filters
    void filter_event(const QString&);              // Text filters
    void filter_event(FilterType, const QString&);  // Tag filters
    void filter_event(RecordTitleItem*, bool);      // Title filters
    void filter_event(int);                         // Date filters
    void filter_event(const QMap<int, int>&);       // Logs checking
    void filter_event(FilterType);                  // Size filters
    void lay_previews(int page = 1);
    void lay_titles();
    void clear_grid(QLayout* layout, bool hide = true);

signals:
    void reroll_response(int);

private:
    Ui::MainWindow *ui;
    VK_Manager* manager;
    Mode current_mode = IDLE;
    View current_view = MAIN;
    QMap<Directories, QString> locations;
    QMap<QString, int> album_ids;
    QDir dir;
    QMap<int, QString> title_map;
    QMap<QString, QString> title_captions;
    QStringList pics;                               // JC, TR
    QStringList quotes;                             // JC, TR
    QVector<int> photo_ids;                         // JC
    QStringList links;                              // JC
    QMap<QString, HashtagButton*> hashtags;
    QList<QStringList> ranked_hashtags;
    QStringList current_hashtags;
    QMap<int, QStringList> hashtags_by_index;
    QMap<QString, Hashtag> full_hashtags_map;
    QMap<QString, HashtagPreview*> selected_hashtags;
    QMap<int, RecordBase*> filtration_results;
    QList<RecordPreview*> selected_records;
    QMap<QString, FilterType> filters;
    QVector<Record> records;
    QList<RecordBase*> record_items;
    QList<RecordBase*> title_items;
    QMap<int, int> records_by_photo_ids;
    QMap<int, int> logs;
    QMap<QString, int> poll_logs;
    int pic_index;
    int quote_index;
    int pic_end_index = 0;
    bool record_edited = false;
    QSet<QPair<int, int>> edited_ranges;
    QStringList subs;
    QMutex status_mutex;
    int post_counter = 0;
    QMap<int, QString> captions_for_ids;

    // Setup functions
    bool initialize();
    void clear_all();
    void set_mode(Mode);
    void set_view(View);
    void set_enabled(bool);
    void set_edited();
    QPixmap scaled(const QImage& source) const;
    QJsonObject json_object(const QString&) const;
    bool save_json(const QJsonObject&, QFile&) const;
    bool data_ready();
    void show_status();
    void keyPressEvent(QKeyEvent*) override;
    void keyReleaseEvent(QKeyEvent*) override;
    QString filtration_indices() const;
    QString filtration_message(int) const;
    QString path(int index);
    QString title_name(int);
    QPair<int, int> title_range(int);
    void save_changes();
    int random_index() const;
    void add_caption(const QString& captcha_sid = "", const QString& captcha_key = "");
    void check_titles(bool);

    // Hashtag management and filtering
    void get_hashtags();
    void create_hashtag_button(const QString&);
    void update_hashtag_grid();
    void load_hashtag_info();
    void update_hashtag_info();
    void update_current_hashtags();
    void recalculate_hashtags(bool);
    QRegularExpressionMatchIterator hashtag_match(const QString&) const;
    void highlight_current_hashtags(bool);
    QString preprocessed(const QString&) const;
    void filter(const QSet<int>&);
    void update_filters(FilterType, const QString&);
    void apply_first_filter();
    void apply_filters();
    void show_filtering_results();
    void exit_filtering();
    QSet<int> word_search(const QString&);
    QSet<int> records_by_public(bool);
    QSet<int> checked_title_records();
    QSet<int> records_by_date(int);
    QSet<int> records_by_size(FilterType);
    void read_poll_logs();
    void update_poll_logs();
    void update_hashtag_file();
    void change_selected_hashtag(const QString&, HashtagPreview*);
    void create_hashtag_preview_connections(const QString&);

    // Screenshot management
    bool load_albums(const QJsonObject&);
    void get_ids(const QJsonObject&);
    bool read_quote_file(QFile&);
    bool update_quote_file(const QString&);
    bool update_quote_file(int, int);
    void register_record();
    void update_record();
    bool open_title_journal(bool all = false);
    bool open_public_journal();
    void read_title_journal(const QJsonObject&);
    void save_title_journal(const QString&);
    void save_title_journal(int, int);
    void read_descriptions(const QJsonObject&);
    QMultiMap<QString, QTime> timestamps_multimap();
    bool find_lines_by_timestamps(const QMultiMap<QString, QTime>&);
    bool get_subs_for_pic();
    void compile_journals();
    void export_text();
    void export_captions_by_ids();
    void export_info_by_ids();
    QJsonObject reverse_index(const QJsonArray&);
    void refactor_journals();
    void display(int);
    void draw(int);
    void show_text(int);
    QString attachments(int) const;
    QString options() const;
    QString poll_message() const;
    void read_logs();
    void update_logs();
};

#endif // MAINWINDOW_H
