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
#include <QPushButton>
#include "vk_manager.h"

namespace Ui {
class MainWindow;
}
class HashtagButton;

class MainWindow : public QMainWindow
{
    Q_OBJECT
    struct Record {
        QString quote;
        QStringList pics;
        QList<int> ids;
        QStringList links;
        bool is_public;
        QJsonObject to_json() const;
    };
    enum Mode {
        IDLE,
        CONFIG_CREATION,
        CONFIG_READING,
        FILTRATION
    };

public:
    explicit MainWindow(QWidget *parent = nullptr);
    ~MainWindow() override;

public slots:
    void hashtag_event(const QChar&, const QString&);
    void filter_event(const QChar&, const QString&);

private:
    Ui::MainWindow *ui;
    VK_Manager* manager;
    QPushButton* add_tag_button;
    const QString group_id = "42265360";
    int client_id;
    Mode current_mode = IDLE;
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
    QStringList current_hashtags;
    QMap<int, QStringList> hashtags_by_index;
    QMap<int, bool> filtration_results;
    QSet<QString> filters;
    QVector<Record> records;
    int pic_index;
    int quote_index;
    int pic_end_index = 0;
    bool record_edited = false;
    bool config_edited = false;
    QSet<QChar> sign_set = {'#', '&', ' '};

    // Setup functions
    void initialize();
    void clear_all();
    void set_mode(Mode);
    void set_enabled(bool);
    void set_edited();
    QPixmap scaled(const QImage& source);
    QJsonObject json_object(const QString&);
    bool save_json(const QJsonObject&, QFile&);
    bool data_ready();
    void show_status();
    void keyPressEvent(QKeyEvent*) override;
    void keyReleaseEvent(QKeyEvent*) override;
    QString filtration_indices();
    QString filtration_message(int);

    // Hashtag management
    void get_hashtags();
    void create_hashtag_button(const QString&);
    void update_hashtag_grid();
    void load_hashtag_info();
    void update_hashtag_info();
    void update_current_hashtags();
    void recalculate_hashtags(bool);
    QRegularExpressionMatchIterator hashtag_match(const QString&);
    void highlight_current_hashtags(bool);
    QString preprocessed(const QString&);
    void filter(const QSet<int>&);
    void update_filters(const QChar&, const QString&);
    QChar parallel_filter_sign(const QChar&, const QString&);
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

class HashtagButton : public QPushButton
{
    Q_OBJECT
public:
    HashtagButton(const QString&);
    ~HashtagButton() override {}
    void mousePressEvent(QMouseEvent*) override;
    void highlight(const QChar&, bool);
    void show_count();
    void reset();
    void add_index(const QChar&, int);
    void remove_index(const QChar&, int);
    QSet<int> indices(const QChar&) const;
signals:
    void filterEvent(const QChar&, const QString&);
    void hashtagEvent(const QChar&, const QString&);
private:
    QString text;
    int count = 0;
    QMap<QChar, QSet<int>> record_indices;
};

#endif // MAINWINDOW_H
