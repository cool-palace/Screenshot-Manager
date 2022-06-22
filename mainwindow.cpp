#include "mainwindow.h"
#include "ui_mainwindow.h"
#include <QDebug>

MainWindow::MainWindow(QWidget *parent) :
    QMainWindow(parent),
    ui(new Ui::MainWindow)
{
    ui->setupUi(this);
    load();

    connect(ui->open_folder, &QAction::triggered, [this]() {
        if (ui->ok->isEnabled()) {
            quotes.clear();
        }
        pics.clear();
        dir = QDir(QFileDialog::getExistingDirectory(nullptr, "Открыть папку с кадрами", screenshots_location));
        pics = dir.entryList(QDir::Files | QDir::NoDotAndDotDot, QDir::Time | QDir::Reversed);

        QFile file(quotes_location + dir.dirName() + ".txt");
        if (!quotes_location.isEmpty()) {
            read_quote_file(file);
        }
        show_status();
        set_enabled(!quotes.empty());
        if (!pics.empty()) {
            draw(0);
        }
    });

    connect(ui->open_doc, &QAction::triggered, [this]() {
        if (ui->ok->isEnabled()) {
            pics.clear();
        }
        quotes.clear();
        auto filepath = QFileDialog::getOpenFileName(nullptr, "Открыть файл с цитатами",
                                                     quotes_location,
                                                     "Текстовые документы (*.txt)");
        QFile file(filepath);
        read_quote_file(file);
        show_status();
    });

    connect(ui->skip, &QPushButton::clicked, [this]() {
        pic_index = qMax(pic_index, pic_end_index) + 1;
        draw(pic_index);
    });

    connect(ui->add, &QPushButton::clicked, [this]() {
        pic_end_index = qMax(pic_index, pic_end_index) + 1;
        draw(pic_end_index);
    });

    connect(ui->ok, &QPushButton::clicked, [this]() {
        register_record();
        pic_index = qMax(pic_index, pic_end_index) + 1;

        if (pic_index < pics.size()) {
            draw(pic_index);
            show_text(++quote_index);
        } else {
            save_title_config();
        }
    });
}

MainWindow::~MainWindow() {
    delete ui;
}

void MainWindow::load() {
    QFile config("config.json");
    if (!config.open(QIODevice::ReadOnly | QIODevice::Text)) {
        ui->statusBar->showMessage("Не удалось найти конфигурационный файл.");
        return;
    }
    QString s = config.readAll();
    config.close();
    QJsonDocument doc = QJsonDocument::fromJson(s.toUtf8());
    QJsonObject json_file = doc.object();
    if (!json_file.contains("screenshots") || !json_file.contains("docs")) {
        ui->statusBar->showMessage("Неверный формат конфигурационного файла.");
        return;
    }
    screenshots_location = json_file.value("screenshots").toString();
    quotes_location = json_file.value("docs").toString();
    if (!QDir(screenshots_location).exists() || !QDir(quotes_location).exists()) {
        screenshots_location = QString();
        quotes_location = QString();
        ui->statusBar->showMessage("Указаны несуществующие директории. Перепроверьте конфигурационный файл.");
        return;
    }
    ui->statusBar->showMessage("Конфигурация успешно загружена.");
}

void MainWindow::register_record() {
    QJsonObject record;
    if (pic_index < pic_end_index) {
        QJsonArray pic_array;
        for (int i = pic_index; i <= pic_end_index; ++i) {
            pic_array.push_back(pics[i]);
        }
        record["filename"] = pic_array;
    } else {
        record["filename"] = pics[pic_index];
    }
    record["caption"] = ui->text->toPlainText();
    record["public"] = true;
    record_array.push_back(record);
}

void MainWindow::read_quote_file(QFile& file) {
    if (!file.open(QIODevice::ReadOnly)) {
        ui->statusBar->showMessage("Не удалось открыть файл с цитатами.");
        return;
    }
    QTextStream in(&file);
    in.setCodec("UTF-8");
    while (!in.atEnd()) {
        quotes.push_back(in.readLine());
    }
    file.close();
    if (!quotes.empty()) {
        show_text(0);
    }
    set_enabled(!pics.empty());
}

void MainWindow::save_title_config() {
    QFile file(dir.dirName() + ".json");
    if (!file.open(QIODevice::WriteOnly | QIODevice::Truncate)) {
        return;
    }
    QJsonObject object;
    object["screens"] = record_array;
    QTextStream out(&file);
    out << QJsonDocument(object).toJson();
    file.close();
    ui->statusBar->showMessage("Конфигурационный файл сохранён.");
}

void MainWindow::set_enabled(bool enable) {
    ui->back->setEnabled(enable);
    ui->ok->setEnabled(enable);
    ui->skip->setEnabled(enable);
    ui->add->setEnabled(enable);
    ui->text->setEnabled(enable);
    ui->text_back->setEnabled(enable);

    if (!enable) return;
    quote_index = pic_index = pic_end_index = 0;
}

QPixmap MainWindow::scaled(const QImage& source) {
    return QPixmap::fromImage(source.scaled(ui->image->geometry().size(), Qt::KeepAspectRatio));
}

void MainWindow::draw(int index) {
    if (pics.size() <= index) return;
    auto image = QImage(dir.path() + QDir::separator() + pics[index]);
    ui->image->setPixmap(scaled(image));
}

void MainWindow::show_text(int index) {
    if (quotes.size() <= index) return;
    ui->text->setText(quotes[index]);
}

void MainWindow::show_status() {
    if (!pics.empty() && quotes.empty()) {
        ui->statusBar->showMessage("Загружено " + QString().setNum(pics.size()) + " кадров. Откройте документ с цитатами.");
    } else if (pics.empty() && !quotes.empty()) {
        ui->statusBar->showMessage("Загружено " + QString().setNum(quotes.size()) + " цитат. Откройте папку с кадрами.");
    } else {
        ui->statusBar->showMessage("Загружено кадров: " + QString().setNum(pics.size()) + ", цитат: " + QString().setNum(quotes.size()) + ".");
    }
}
