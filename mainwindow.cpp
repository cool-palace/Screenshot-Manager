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
        dir = QDir(QFileDialog::getExistingDirectory(nullptr, "Открыть папку с кадрами",
                                                     screenshots_location));
        pics = dir.entryList(QDir::Files | QDir::NoDotAndDotDot);

        QFile file(quotes_location + dir.dirName() + ".txt");
        if (!quotes_location.isEmpty() && read_quote_file(file)) {
            set_mode(CONFIG_CREATION);
            set_enabled(current_mode);
        }
        show_status();
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

    connect(ui->open_json, &QAction::triggered, [this]() {
        if (ui->ok->isEnabled()) {
            pics.clear();
            quotes.clear();
        }
        auto filepath = QFileDialog::getOpenFileName(nullptr, "Открыть json-файл",
                                                     QCoreApplication::applicationDirPath(),
                                                     "Файлы (*.json)");
        auto json_file = json_object(filepath);
        if (!json_file.contains("title") || !json_file.contains("screens")) {
            ui->statusBar->showMessage("Неверный формат конфигурационного файла.");
            return;
        }
        read_title_config(json_file);

        if (!records.empty()) {
            set_mode(CONFIG_READING);
            set_enabled(current_mode);
            display(0);
        }
        show_status();
    });

    connect(ui->skip, &QPushButton::clicked, [this]() {
        pic_index = qMax(pic_index, pic_end_index) + 1;
        draw(pic_index);
    });

    connect(ui->back, &QPushButton::clicked, [this]() {
        switch (current_mode) {
        case CONFIG_CREATION:
            if (pic_index == pic_end_index) {
                pic_end_index = 0;
            }
        {
            int& current_index = pic_index > pic_end_index ? pic_index : pic_end_index;
            if (current_index == 0) break;
            draw(--current_index);
        }
            break;
        case CONFIG_READING:
            pic_end_index = 0;
            display(--pic_index);
            break;
        default:
            break;
        }
    });

    connect(ui->add, &QPushButton::clicked, [this]() {
        switch (current_mode) {
        case CONFIG_CREATION:
            pic_end_index = qMax(pic_index, pic_end_index) + 1;
            draw(pic_end_index);
            break;
        case CONFIG_READING:
            ++pic_end_index;
            display(pic_index);
            break;
        default:
            break;
        }
    });

    connect(ui->ok, &QPushButton::clicked, [this]() {
        switch (current_mode) {
        case CONFIG_CREATION:
            register_record();
            pic_index = qMax(pic_index, pic_end_index) + 1;
            pic_end_index = 0;
            ui->make_private->setChecked(false);
            if (pic_index < pics.size()) {
                draw(pic_index);
                show_text(++quote_index);
            } else {
                save_title_config();
                set_mode(IDLE);
            }
            break;
        case CONFIG_READING:
            pic_end_index = 0;
            display(++pic_index);
            break;
        default:
            break;
        }
    });
}

MainWindow::~MainWindow() {
    delete ui;
}

void MainWindow::set_mode(Mode mode) {
    current_mode = mode;
    quote_index = pic_index = pic_end_index = 0;
    switch (mode) {
    case CONFIG_CREATION:
        ui->ok->setText("Готово");
        ui->add->setText("Добавить");
        break;
    case CONFIG_READING:
        ui->ok->setText("Далее");
        ui->add->setText("Листать");
        break;
    default:
        break;
    }
}

void MainWindow::load() {
    auto json_file = json_object("config.json");
    if (!json_file.contains("screenshots") || !json_file.contains("docs")) {
        ui->statusBar->showMessage("Не удалось прочитать конфигурационный файл.");
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

QJsonObject MainWindow::json_object(const QString& filepath) {
    QFile config(filepath);
    if (!config.open(QIODevice::ReadOnly | QIODevice::Text)) {
        return QJsonObject();
    }

    QString s = QString::fromUtf8(config.readAll());
    config.close();
    QJsonDocument doc = QJsonDocument::fromJson(s.toUtf8());
    QJsonObject json_file = doc.object();
    return json_file;
}

void MainWindow::register_record() {
    QJsonObject record;
    QJsonArray pic_array;
    for (int i = pic_index; i <= qMax(pic_index, pic_end_index); ++i) {
        pic_array.push_back(pics[i]);
    }
    record["filename"] = pic_array;
    record["caption"] = ui->text->toPlainText();
    record["public"] = !ui->make_private->isChecked();
    record_array.push_back(record);
}

bool MainWindow::read_quote_file(QFile& file) {
    if (!file.open(QIODevice::ReadOnly)) {
        ui->statusBar->showMessage("Не удалось открыть файл с цитатами.");
        return false;
    }
    QTextStream in(&file);
    in.setCodec("UTF-8");
    while (!in.atEnd()) {
        quotes.push_back(in.readLine());
    }
    file.close();
    if (!quotes.empty()) {
        show_text(0);
        set_enabled(!pics.empty());
        return true;
    } else return false;
}

void MainWindow::read_title_config(const QJsonObject& json_file) {
    auto title = json_file.value("title").toString();
    dir = QDir(screenshots_location + title);
    auto records_array = json_file.value("screens").toArray();

    for (QJsonValueRef r : records_array) {
        Record record;
        auto object = r.toObject();
        record.quote = object["caption"].toString();
        record.is_public = object["public"].toBool();
        auto filename_array = object["filename"].toArray();
        for (QJsonValueRef name : filename_array) {
            record.pics.push_back(name.toString());
        }
        records.push_back(record);
    }
}

void MainWindow::save_title_config() {
    QFile file(dir.dirName() + ".json");
    if (!file.open(QIODevice::WriteOnly | QIODevice::Truncate)) {
        return;
    }
    QJsonObject object;
    object["title"] = dir.dirName();
    object["screens"] = record_array;
    QTextStream out(&file);
    out.setCodec("UTF-8");
    out << QJsonDocument(object).toJson();
    file.close();
    ui->statusBar->showMessage("Конфигурационный файл сохранён.");
}

void MainWindow::set_enabled(bool enable) {
    ui->back->setEnabled(enable);
    ui->ok->setEnabled(enable);
    ui->skip->setEnabled(enable && current_mode == CONFIG_CREATION);
    ui->add->setEnabled(enable);
    ui->text->setEnabled(enable);
    ui->make_private->setEnabled(enable);
}

QPixmap MainWindow::scaled(const QImage& source) {
    return QPixmap::fromImage(source.scaled(ui->image->geometry().size(), Qt::KeepAspectRatio));
}

void MainWindow::display(int index) {
    auto image = QImage(dir.path() + QDir::separator() + records[index].pics[pic_end_index]);
    ui->image->setPixmap(scaled(image));
    ui->text->setText(records[index].quote);
    bool reached_end = index + 1 >= records.size();
    bool listing_on = pic_end_index + 1 < records[index].pics.size();
    ui->add->setEnabled(listing_on && !reached_end);
    ui->ok->setEnabled(!reached_end);
    ui->back->setEnabled(index > 0);
}

void MainWindow::draw(int index) {
    auto image = QImage(dir.path() + QDir::separator() + pics[index]);
    ui->image->setPixmap(scaled(image));
    bool reached_end = index + 1 >= pics.size();
    ui->skip->setEnabled(!reached_end);
    ui->add->setEnabled(!reached_end);
    ui->back->setEnabled(index > 0);
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
    } else if (records.empty()) {
        ui->statusBar->showMessage("Загружено кадров: " + QString().setNum(pics.size()) + ", цитат: " + QString().setNum(quotes.size()) + ".");
    } else {
         ui->statusBar->showMessage("Загружено " + QString().setNum(records.size()) + " записей.");
    }
}
