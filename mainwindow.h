#ifndef MAINWINDOW_H
#define MAINWINDOW_H

#include <QMainWindow>
#include <QDir>
#include <QFileDialog>
#include <QLabel>
#include <QJsonArray>
#include <QJsonDocument>
#include <QJsonObject>

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
    Mode current_mode = IDLE;
    void load();
    void register_record();
    void save_title_config();
    void read_title_config(const QJsonObject&);
    bool read_quote_file(QFile&);
    void set_mode(Mode);
    QJsonObject json_object(const QString& filepath);
    QString screenshots_location;
    QString quotes_location;
    QDir dir;
    QStringList pics;
    QStringList quotes;
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
