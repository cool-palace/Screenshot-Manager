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
        CONFIG_CREATION,
        CONFIG_READING,
        TEXT_READING,
        RELEASE_PREPARATION
    };
    enum View {
        MAIN,
        LIST,
        GALLERY
    };
    struct FilterSpecs {
        QChar sign;
        bool include;
        FilterSpecs(const QChar& sign, bool include) : sign(sign), include(include) {}
        FilterSpecs() : sign('#'), include(true) {}
    };

public:
    explicit MainWindow(QWidget *parent = nullptr);
    ~MainWindow() override;

protected:
    void closeEvent(QCloseEvent*) override;

public slots:
    void hashtag_event(const QChar&, const QString&);
    void filter_event(bool);
    void filter_event(const QString&);
    void filter_event(const QChar&, const QString&, bool);
    void lay_previews(int page = 1);

private:
    Ui::MainWindow *ui;
    VK_Manager* manager;
    const QString group_id = "42265360";
    const int pics_per_page = 70;
    int client_id;
    Mode current_mode = IDLE;
    View current_view = MAIN;
    QString screenshots_location;
    QString quotes_location;
    QString configs_location;
    QString subs_location;
    QString access_token;
    QMap<QString, int> album_ids;
    QDir dir;
    QMap<int, QString> title_map;
    QStringList pics;
    QStringList quotes;
    QVector<int> photo_ids;
    QStringList links;
    QMap<QString, HashtagButton*> hashtags;
    QList<QStringList> ranked_hashtags;
    QStringList current_hashtags;
    QMap<int, QStringList> hashtags_by_index;
    QMap<int, RecordBase*> filtration_results;
    QList<RecordBase*> selected_records;
    QMap<QString, FilterSpecs> filters;
    QVector<Record> records;
    QList<RecordBase*> record_items;
    int pic_index;
    int quote_index;
    int pic_end_index = 0;
    bool record_edited = false;
    bool config_edited = false;
    QSet<QPair<int, int>> edited_ranges;
    QSet<QChar> sign_set = {'#', '&', ' ', 't', 'p'};
    QSet<int> all_records;
    QStringList subs;

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
    void update_filters(const QChar&, const QString&, bool);
    void apply_first_filter();
    void apply_filters();
    void show_filtering_results();
    void exit_filtering();
    QSet<int> word_search(const QString&);
    QSet<int> records_by_public(bool);

    // Screenshot management
    bool load_albums(const QJsonObject&);
    void get_ids(const QJsonObject&);
    bool read_quote_file(QFile&);
    bool update_quote_file(const QString&);
    bool update_quote_file(int, int);
    void register_record();
    void update_record();
    bool open_title_config(bool all = false);
    bool open_public_config();
    void read_title_config(const QJsonObject&);
    void save_title_config(const QString&);
    void save_title_config(int, int);
    void read_text_from_subs();
    QMultiMap<QString, QTime> timestamps_multimap();
    bool find_lines_by_timestamps(const QMultiMap<QString, QTime>&);
    bool get_subs_for_pic();
    void compile_configs();
    void export_text();
    QJsonObject reverse_index(const QJsonArray&);
    void refactor_configs();
    void display(int);
    void draw(int);
    void show_text(int);
};

#endif // MAINWINDOW_H
