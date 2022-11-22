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
#include "vk_manager.h"

namespace Ui {
class MainWindow;
}

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
        CONFIG_READING
    };

public:
    explicit MainWindow(QWidget *parent = nullptr);
    ~MainWindow();

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
    QMap<QString, QPushButton*> hashtags;
    QMap<QString, size_t> hashtags_count;
    QStringList current_hashtags;
    QVector<Record> records;
    int pic_index;
    int quote_index;
    int pic_end_index = 0;
    bool record_edited = false;
    bool config_edited = false;

    // Setup functions
    void initialize();
    void get_hashtags();
    void create_hashtag_button(const QString&);
    void update_hashtag_grid();
    void load_hashtag_info();
    void update_hashtag_info();
    void update_current_hashtags();
    void recalculate_hashtags(bool);
    QRegularExpressionMatchIterator hashtag_match(const QString&);
    void highlight_current_hashtags(bool);
    void highlight_button(QPushButton*, bool);
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
