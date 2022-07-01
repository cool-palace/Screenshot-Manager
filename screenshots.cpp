#include "mainwindow.h"
#include "ui_mainwindow.h"
#include <QDebug>

bool MainWindow::load_albums(const QJsonObject& reply) {
    QJsonObject albums_config;
    if (reply.contains("error")) {
        return false;
    }
    auto items = reply["response"].toObject()["items"].toArray();
    for (const QJsonValueRef album : items) {
        auto album_object = album.toObject();
        auto title = album_object["title"].toString();
        int album_id = album_object["id"].toInt();
        album_ids.insert(title, album_id);
    }
    return true;
}

void MainWindow::get_ids(const QJsonObject & reply) {
    auto array = reply["response"].toObject()["items"].toArray();
    for (const QJsonValueRef item : array) {
        auto id = item.toObject()["id"].toInt();
        photo_ids.push_back(id);
    }
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
        return true;
    } else return false;
}

bool MainWindow::update_quote_file() {
    QFile file(quotes_location + dir.dirName() + ".txt");
    if (!file.open(QIODevice::WriteOnly | QIODevice::Truncate)) {
        ui->statusBar->showMessage("Не удалось открыть файл с цитатами.");
        return false;
    }
    QTextStream out(&file);
    out.setCodec("UTF-8");
    for (const auto& record : records) {
        out << record.quote + "\r\n";
    }
    file.close();
    return true;
}

void MainWindow::register_record() {
    Record record;
    for (int i = pic_index; i <= qMax(pic_index, pic_end_index); ++i) {
        record.pics.push_back(pics[i]);
        record.ids.push_back(photo_ids[i]);
    }
    record.quote = ui->text->toPlainText();
    record.is_public = !ui->make_private->isChecked();
    records.push_back(record);
}

void MainWindow::update_record() {
    records[pic_index].quote = ui->text->toPlainText();
    records[pic_index].is_public = !ui->make_private->isChecked();
    record_edited = false;
}

bool MainWindow::open_title_config() {
    clear_all();
    auto filepath = QFileDialog::getOpenFileName(nullptr, "Открыть конфигурационный файл",
                                                 configs_location,
                                                 "Файлы (*.json)");
    auto json_file = json_object(filepath);
    if (!json_file.contains("title") || !json_file.contains("screens")) {
        ui->statusBar->showMessage("Неверный формат файла.");
        return false;
    }
    read_title_config(json_file);
    return !records.empty();
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
        auto filename_array = object["filenames"].toArray();
        for (QJsonValueRef name : filename_array) {
            record.pics.push_back(name.toString());
        }
        auto id_array = object["photo_ids"].toArray();
        for (QJsonValueRef id : id_array) {
            record.ids.push_back(id.toInt());
        }
        records.push_back(record);
    }
}

void MainWindow::save_title_config() {
    QJsonArray record_array;
    for (const auto& record : records) {
        record_array.push_back(record.to_json());
    }
    QFile file(configs_location + dir.dirName() + ".json");
    QJsonObject object;
    object["title"] = dir.dirName();
    object["album_id"] = album_ids[dir.dirName()];
    object["screens"] = record_array;
    auto message = save_json(object, file)
            ? "Конфигурационный файл сохранён."
            : "Не удалось сохранить файл.";
    if (current_mode == CONFIG_READING) {
        config_edited = false;
        ui->skip->setEnabled(false);
    }
    ui->statusBar->showMessage(message);
}

void MainWindow::compile_configs() {
    QDir dir = QDir(configs_location);
    auto configs = dir.entryList(QDir::Files | QDir::NoDotAndDotDot);
    QJsonArray resulting_array;
    for (const auto& config : configs) {
        auto object = json_object(configs_location + config);
        auto title = object["title"].toString();
        auto album_id = object["album_id"].toString();
        auto array = object["screens"].toArray();
        for (QJsonValueRef item : array) {
            auto record = item.toObject();
            record["title"] = title;
            record["album_id"] = album_id;
            resulting_array.push_back(record);
        }
    }
    QJsonObject result;
    result["records"] = resulting_array;
    QFile file("total.json");
    auto message = save_json(result, file)
            ? "Конфигурационный файл скомпилирован."
            : "Не удалось сохранить файл.";
    ui->statusBar->showMessage(message);
}

void MainWindow::display(int index) {
    if (!ui->offline->isChecked()) {
        manager->get_photo(records[index].ids[pic_end_index]);
    } else {
        auto image = QImage(dir.path() + QDir::separator() + records[index].pics[pic_end_index]);
        ui->image->setPixmap(scaled(image));
    }
    disconnect(ui->text, &QTextEdit::textChanged, this, &MainWindow::set_edited);
    ui->text->setText(records[index].quote);
    connect(ui->text, &QTextEdit::textChanged, this, &MainWindow::set_edited);
    bool reached_end = index + 1 >= records.size();
    bool listing_on = pic_end_index + 1 < records[index].pics.size();
    ui->skip->setEnabled(config_edited);
    ui->make_private->setChecked(!records[index].is_public);
    ui->add->setEnabled(listing_on && !reached_end);
    ui->ok->setEnabled(!reached_end);
    ui->back->setEnabled(index > 0);
}

void MainWindow::draw(int index = 0) {
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

QJsonObject MainWindow::Record::to_json() const {
    QJsonObject current_record;
    QJsonArray pic_array, id_array;
    for (int i = 0; i < pics.size(); ++i) {
        pic_array.push_back(pics[i]);
        id_array.push_back(ids[i]);
    }
    current_record["filenames"] = pic_array;
    current_record["photo_ids"] = id_array;
    current_record["caption"] = quote;
    current_record["public"] = is_public;
    return current_record;
}