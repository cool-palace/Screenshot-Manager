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
#include <QImageReader>
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
//        QStringList links;
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
    QVector<Record> records;
    int pic_index;
    int quote_index;
    int pic_end_index = 0;
    bool record_edited = false;
    bool config_edited = false;

    // Setup functions
    void initialize();
    void clear_all();
    void set_mode(Mode);
    void set_enabled(bool);
    void set_edited();
    QPixmap scaled(const QImage& source);
    QJsonObject json_object(const QString&);
    bool save_json(const QJsonObject&, QFile&);
    QJsonObject reply(QNetworkReply*);
    void get_link(const QJsonObject&);
    QImage image(QNetworkReply*);
    bool data_ready();
    void show_status();

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
    void display(int);
    void draw(int);
    void show_text(int);
};

#endif // MAINWINDOW_H
