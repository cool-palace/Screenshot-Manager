#include "mainwindow.h"
#include "ui_mainwindow.h"
#include <QDebug>

MainWindow::MainWindow(QWidget *parent)
    : QMainWindow(parent)
    , ui(new Ui::MainWindow)
{
    ui->setupUi(this);
    if (initialize()) {
//        get_hashtags();
    }

    connect(ui->config_creation, &QAction::triggered, this, &MainWindow::journal_creation);
    connect(ui->config_reading, &QAction::triggered, this, &MainWindow::journal_reading);
    connect(ui->config_reading_all, &QAction::triggered, this, &MainWindow::journal_reading_all);
    connect(ui->text_reading, &QAction::triggered, this, &MainWindow::text_reading);
    connect(ui->descriptions_reading, &QAction::triggered, this, &MainWindow::descriptions_reading);
    connect(ui->release_preparation, &QAction::triggered, this, &MainWindow::release_preparation);

    connect(ui->initialization, &QAction::triggered, this, &MainWindow::initialize);
    connect(ui->compile, &QAction::triggered, this, &MainWindow::compile_journals);
    connect(ui->export_text, &QAction::triggered, this, &MainWindow::export_text);
//    connect(ui->add_hashtag, &QAction::triggered, this, &MainWindow::add_hashtag);

}

MainWindow::~MainWindow() {
    delete ui;
    delete manager;
}

void MainWindow::keyPressEvent(QKeyEvent* event) {
    emit key_press(event);
}

void MainWindow::keyReleaseEvent(QKeyEvent* event) {
    emit key_release(event);
}

void MainWindow::closeEvent(QCloseEvent *event) {
    emit close_event(event);
}

bool MainWindow::initialize() {
    auto json_file = json_object("config.json");
    if (json_file.empty()) {
        auto config_file = QFileDialog::getOpenFileName(nullptr, "Открыть конфигурационный файл",
                                                                 locations[JOURNALS],
                                                                 "Файлы (*.json)");
        json_file = json_object(config_file);
    }
    if (!json_file.contains("screenshots") /*|| !json_file.contains("docs")*/ || !json_file.contains("configs")) {
        ui->statusBar->showMessage("Неверный формат конфигурационного файла.");
        return false;
    }
    locations[SCREENSHOTS] = json_file.value("screenshots").toString();
    locations[SCREENSHOTS_NEW] = locations[SCREENSHOTS] + "Новые кадры\\";
    locations[QUOTES] = json_file.value("docs").toString();
    locations[SUBS] = json_file.value("subs").toString();
    locations[JOURNALS] = json_file.value("configs").toString();
    locations[LOGS_FILE] = json_file.value("logs").toString();
    locations[POLL_LOGS] = json_file.value("poll_logs").toString();
    QString access_token = json_file.value("access_token").toString();
    QString group_id = json_file.value("group_id").toString();
    QString public_id = json_file.value("public_id").toString();
    manager = new VK_Manager(access_token, group_id, public_id);
    RecordFrame::manager = manager;
    if (!QDir(locations[SCREENSHOTS]).exists() || !QDir(locations[JOURNALS]).exists()) {
        ui->statusBar->showMessage("Указаны несуществующие директории. Перепроверьте конфигурационный файл.");
        return false;
    }
    ui->statusBar->showMessage("Конфигурация успешно загружена.");
    return true;
}
