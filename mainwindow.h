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
    Mode current_mode = IDLE;
    QString screenshots_location;
    QString quotes_location;
    QString configs_location;
    QString access_token;
    QMap<QString, int> album_ids;
    QDir dir;
    QStringList pics;
    QStringList quotes;
    QStringList urls; //
    QVector<int> photo_ids;
    QVector<Record> records;
    int pic_index;
    int quote_index;
    int pic_end_index = 0;

    QJsonObject reply(QNetworkReply*);
    QImage image(QNetworkReply*);
    void clear_all();
    void load();
    void register_record();
    void save_title_config();
    void read_title_config(const QJsonObject&);
    bool read_quote_file(QFile&);
    bool update_quote_file();
    bool open_json();
    bool save_json(const QJsonObject&, QFile&);
    void save_albums(const QJsonObject&);
    void load_albums();
    void load_albums(const QJsonObject&);
    void get_urls(const QJsonObject&);
    void get_ids(const QJsonObject&);
    void set_mode(Mode);
    QString album_id(const QString&);
    QJsonObject json_object(const QString&);
    QPixmap scaled(const QImage& source);
    void set_enabled(bool);
    void display(int);
    void draw(int);
    void show_text(int);
    bool initialization_status();
    bool read_config_status();
    void show_status();
};

#endif // MAINWINDOW_H
