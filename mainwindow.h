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
        FILTRATION
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
    void filter_event(const QChar&, const QString&, bool);

private:
    Ui::MainWindow *ui;
    VK_Manager* manager;
    const QString group_id = "42265360";
    int client_id;
    Mode current_mode = IDLE;
    View current_view = MAIN;
    QString screenshots_location;
    QString quotes_location;
    QString configs_location;
    QString access_token;
    QMap<QString, int> album_ids;
    QDir dir;
    QStringList pics;
    QStringList quotes;
    QVector<int> photo_ids;
    QStringList links;
    QMap<QString, HashtagButton*> hashtags;
    QList<QStringList> ranked_hashtags;
    QStringList current_hashtags;
    QMap<int, QStringList> hashtags_by_index;
    QMap<int, RecordItem*> filtration_results;
    QMap<QString, FilterSpecs> filters;
    QVector<Record> records;
//    RecordItem* record_items_array;
    QList<RecordItem*> record_items;
    int pic_index;
    int quote_index;
    int pic_end_index = 0;
    bool record_edited = false;
    bool config_edited = false;
    QSet<QChar> sign_set = {'#', '&', ' '};
    QSet<int> all_records;

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
    QString path() const { return dir.path() + QDir::separator(); }

    // Hashtag management
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
    void show_filtering_results();
    void exit_filtering();

    // Screenshot management
    bool load_albums(const QJsonObject&);
    void get_ids(const QJsonObject&);
    bool read_quote_file(QFile&);
    bool update_quote_file();
    void register_record();
    void update_record();
    bool open_title_config();
    void read_title_config(const QJsonObject&);
    void save_title_config();
    void compile_configs();
    QJsonObject reverse_index(const QJsonArray&);
    void refactor_configs();
    void display(int);
    void draw(int);
    void show_text(int);
};

#endif // MAINWINDOW_H
