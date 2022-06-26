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

namespace Ui {
class MainWindow;
}

class MainWindow : public QMainWindow
{
    Q_OBJECT
    struct Record {
        QString quote;
        QStringList pics;
        QStringList links;
        bool is_public;
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
    QNetworkAccessManager* manager;
    const QString group_id = "42265360";
    Mode current_mode = IDLE;
    void get_reply(QNetworkReply*);
    void load();
    void register_record();
    void save_title_config();
    void read_title_config(const QJsonObject&);
    bool read_quote_file(QFile&);
    bool save_json(const QJsonObject&, QFile&);
    void save_albums(const QJsonObject& );
    void set_mode(Mode);
    void get_urls(const QJsonObject&);
    QJsonObject json_object(const QString& filepath);
    QString screenshots_location;
    QString quotes_location;
    QString presets_location;
    QString access_token;
    QDir dir;
    QStringList pics;
    QStringList quotes;
    QStringList urls;
    QVector<Record> records;
    QJsonArray record_array;
    int pic_index;
    int quote_index;
    int pic_end_index = 0;
    QPixmap scaled(const QImage& source);
    void set_enabled(bool);
    void display(int);
    void draw(int);
    void show_text(int);
    void show_status();
};

#endif // MAINWINDOW_H
