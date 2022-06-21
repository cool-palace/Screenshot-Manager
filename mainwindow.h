#ifndef MAINWINDOW_H
#define MAINWINDOW_H

#include <QMainWindow>
#include <QDir>
#include <QFileDialog>
#include <QLabel>

namespace Ui {
class MainWindow;
}

class MainWindow : public QMainWindow
{
    Q_OBJECT

public:
    explicit MainWindow(QWidget *parent = nullptr);
    ~MainWindow();

private:
    Ui::MainWindow *ui;
    QDir dir;
    QStringList pics;
    QStringList quotes;
    int pic_index;
    int quote_index;
    int pic_end_index = 0;
    QPixmap scaled(const QImage& source);
    void set_enabled(bool);
    void draw(int);
    void show_text(int);
    void show_status();
};

#endif // MAINWINDOW_H
