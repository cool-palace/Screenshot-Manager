#include "mainwindow.h"
#include "ui_mainwindow.h"
#include <QDebug>
#include "journal_creation.h"
#include "text_reading.h"
#include "journal_reading.h"
#include "release_preparation.h"

void MainWindow::journal_creation() {
    if (mode) {
        qDebug() << mode;
        mode->~AbstractMode();
        mode = new(mode) JournalCreation(this);
    } else mode = new JournalCreation(this);
    mode->start();
}

void MainWindow::journal_reading() {
    if (mode) {
        qDebug() << mode;
        mode->~AbstractMode();
        mode = new(mode) JournalReading(this);
    } else mode = new JournalReading(this);
    mode->start();
}

void MainWindow::journal_reading_all() {
    if (mode) {
        qDebug() << mode;
        mode->~AbstractMode();
        mode = new(mode) JournalReading(this, true);
    } else mode = new JournalReading(this, true);
    mode->start();
}

void MainWindow::text_reading() {
    if (mode) {
        qDebug() << mode;
        mode->~AbstractMode();
        mode = new(mode) TextReading(this);
    } else mode = new TextReading(this);
    mode->start();
}

void MainWindow::descriptions_reading() {
//    clear_all();
//    auto path = QFileDialog::getOpenFileName(nullptr, "Открыть файл с описаниями",
//                                                       locations[JOURNALS] + "//descriptions",
//                                                       "Файлы (*.json)");
//    read_descriptions(json_object(path));
//    if (records.isEmpty()/* quotes.isEmpty() || pics.isEmpty()*/) {
//        set_mode(IDLE);
//        ui->statusBar->showMessage("Не удалось прочесть описания из файла.");
//        return;
//    }
//    set_mode(DESCRIPTION_READING);
//    set_view(MAIN);
}

void MainWindow::release_preparation() {
    if (mode) {
        qDebug() << mode;
        mode->~AbstractMode();
        mode = new(mode) ReleasePreparation(this);
    } else mode = new ReleasePreparation(this);
    mode->start();
}

