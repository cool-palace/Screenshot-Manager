#ifndef MAINWINDOW_H
#define MAINWINDOW_H

#include <QMainWindow>
#include <QNetworkAccessManager>
#include <QNetworkReply>
#include <QDir>
#include <QFileDialog>
#include <QLabel>
#include <QJsonArray>
#include <QKeyEvent>
#include <QInputDialog>
#include <QRegularExpression>
#include <QMessageBox>
#include <QProgressDialog>
#include "include\vk_manager.h"
#include "include\hashtag_button.h"
#include "include\record.h"
#include "include\recordpreview.h"
#include "include\common.h"
#include "include\database.h"

namespace Ui {
class MainWindow;
}

class AbstractMode;
class JournalCreation;
class TextReading;
class JournalReading;
class ReleasePreparation;

enum Directories {
    JOURNALS,
    SCREENSHOTS,
    SCREENSHOTS_NEW,
    QUOTES,
    SUBS,
    SUBS_NEW,
    HASHTAGS,
    LOGS_FILE,
    POLL_LOGS,
    PUBLIC_RECORDS,
    HIDDEN_RECORDS,
    DATABASE
};

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

public:
    explicit MainWindow(QWidget *parent = nullptr);
    ~MainWindow() override;
    Ui::MainWindow* get_ui() { return ui; }
    VK_Manager* get_vk() { return manager; }
    QMap<Directories, QString> get_locations() { return locations; };

protected:
    void resizeEvent(QResizeEvent *event) override;
    void closeEvent(QCloseEvent*) override;

signals:
    void key_press(QKeyEvent*);
    void key_release(QKeyEvent*);
    void close_event(QCloseEvent*);

public slots:
    // Launching modes
    void journal_creation();
    void journal_reading();
    void journal_reading_all();
    void text_reading();
    void descriptions_reading();
    void release_preparation();

private:
    Ui::MainWindow *ui;
    VK_Manager* manager;
    QMap<Directories, QString> locations;
    AbstractMode* mode = nullptr;
    Mode current_mode = IDLE;
    ProgressDialog* progress_dialog;

    // Setup functions
    bool initialize();
    void clear_all();
    void set_mode(Mode);
    void keyPressEvent(QKeyEvent*) override;
    void keyReleaseEvent(QKeyEvent*) override;
    void exit_mode();

    // Screenshot management
    void read_descriptions(const QJsonObject&);
    void compile_journals();
    void compile_journals_to_db();
    void export_text();
    QJsonObject reverse_index(const QJsonArray&);
    void refactor_journals();
};

#endif // MAINWINDOW_H
