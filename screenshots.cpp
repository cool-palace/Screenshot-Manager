#include "mainwindow.h"
#include "ui_mainwindow.h"
#include <QDebug>

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
        record.links.push_back(links[i]);
    }
    record.quote = ui->text->toPlainText();
    record.is_public = !ui->make_private->isChecked();
    records.push_back(record);
}

void MainWindow::update_record() {
    records[pic_index].quote = ui->text->toPlainText();
    records[pic_index].is_public = !ui->make_private->isChecked();
    record_edited = false;
    update_hashtag_info();
}

bool MainWindow::open_title_config() {
    clear_all();
    auto filepath = QFileDialog::getOpenFileName(nullptr, "Открыть конфигурационный файл",
                                                 configs_location,
                                                 "Файлы (*.json)");
    if (filepath.isEmpty()) return false;
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
        record.quote = preprocessed(object["caption"].toString());
        record.is_public = object["public"].toBool();
        auto filename_array = object["filenames"].toArray();
        for (QJsonValueRef name : filename_array) {
            record.pics.push_back(name.toString());
        }
        auto id_array = object["photo_ids"].toArray();
        for (QJsonValueRef id : id_array) {
            record.ids.push_back(id.toInt());
        }
        auto link_array = object["links"].toArray();
        for (QJsonValueRef link : link_array) {
            record.links.push_back(link.toString());
        }
        records.push_back(record);
    }
    for (int i = 0; i < records.size(); ++i) {
        record_items.push_back(new RecordItem(records[i], i, path()));
        connect(record_items[i], &RecordItem::selected, [this](int index){
            ui->slider->setValue(index);
            set_view(MAIN);
        });
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

void MainWindow::read_text_from_subs() {
    dir = QDir(QFileDialog::getExistingDirectory(nullptr, "Открыть папку с кадрами",
                                                           screenshots_location));
    pics = dir.entryList(QDir::Files | QDir::NoDotAndDotDot);
    auto timestamps_for_filenames = timestamps_multimap();
    if (timestamps_for_filenames.isEmpty()) return;
    find_lines_by_timestamps(timestamps_for_filenames);
    update_quote_file();
    QString message = "Записан файл " + dir.dirName() + ".txt";
    ui->statusBar->showMessage(message);
    clear_all();
}

QMultiMap<QString, QTime> MainWindow::timestamps_multimap() {
    QMultiMap<QString, QTime> timestamps_for_filenames;
    int ms_offset = -250;
    for (const auto& pic : pics) {
        QRegularExpression regex("(.*)?-(\\d-\\d\\d-\\d\\d-\\d{3})");
        auto i = regex.globalMatch(pic);
        if (i.hasNext()) {
            auto match = i.next();
            auto filename = match.captured(1);
            auto time = QTime::fromString(match.captured(2), "h-mm-ss-zzz").addMSecs(ms_offset);
            timestamps_for_filenames.insert(filename, time);
        }
    }
    return timestamps_for_filenames;
}

void MainWindow::find_lines_by_timestamps(const QMultiMap<QString, QTime>& timestamps_for_filenames) {
    auto dir_subs = QDir(QFileDialog::getExistingDirectory(nullptr, "Открыть папку с cубтитрами",
                                                           screenshots_location));
    for (const auto& filename : timestamps_for_filenames.uniqueKeys()) {
        auto path = QDir::toNativeSeparators(dir_subs.path()) + QDir::separator() + filename + ".ass";
        QFile file(path);
        if (!file.open(QIODevice::ReadOnly)) {
            QString message = "Ошибка при чтении субтитров: не удалось открыть файл " + file.fileName();
             ui->statusBar->showMessage(message);
            return;
        }
        auto timestamps = timestamps_for_filenames.values(filename);
        QTextStream in(&file);
        in.setCodec("UTF-8");
        QRegularExpression regex("Dialogue: 0,(\\d:\\d\\d:\\d\\d\\.\\d\\d),(\\d:\\d\\d:\\d\\d\\.\\d\\d),.+,0,0,0,,(.+)");
        QString last_line;
        while (!timestamps.isEmpty() && !in.atEnd()) {
            auto line = in.readLine();
            auto i = regex.globalMatch(line);
            auto time = timestamps.last();
            if (i.hasNext()) {
                auto match = i.next();
                auto line_start = QTime::fromString(match.captured(1), "h:mm:ss.z");
                auto line_finish = QTime::fromString(match.captured(2), "h:mm:ss.z");
                bool time_within_bounds = time <= line_finish && time >= line_start;
                bool time_missed = time < line_start && time.addSecs(10) > line_start;
                if (time_within_bounds || time_missed) {
                    records.append(Record(time_within_bounds ? match.captured(3) : last_line));
                    timestamps.pop_back();
                }
                last_line = match.captured(3);
            }
        }
        file.close();
    }
}

void MainWindow::compile_configs() {
    QDir dir = QDir(configs_location);
    auto configs = dir.entryList(QDir::Files | QDir::NoDotAndDotDot);
    QJsonArray resulting_array;
    QJsonArray hidden_array;
    for (const auto& config : configs) {
        auto object = json_object(configs_location + config);
//        auto title = object["title"].toString();
//        auto album_id = object["album_id"].toString();
        auto array = object["screens"].toArray();
        for (QJsonValueRef item : array) {
            auto record = item.toObject();
//                record["title"] = title;
//                record["album_id"] = album_id;
            (record["public"].toBool() ? resulting_array : hidden_array).push_back(record);
        }
    }
    QJsonObject result, hidden_result;
    result["records"] = resulting_array;
    result["reverse_index"] = reverse_index(resulting_array);
    hidden_result["records"] = hidden_array;
    hidden_result["reverse_index"] = reverse_index(hidden_array);
    QFile file(configs_location + "result\\public_records.json");
    QFile hidden_file(configs_location + "result\\hidden_records.json");
    save_json(hidden_result, hidden_file);
    auto message = save_json(result, file)
            ? "Обработано конфигов: " + QString().setNum(configs.size()) + ", "
              "скомпилировано публичных записей: " + QString().setNum(resulting_array.size()) + ", "
              "скрытых записей: " + QString().setNum(hidden_array.size())
            : "Не удалось сохранить файл.";
    ui->statusBar->showMessage(message);
}

QJsonObject MainWindow::reverse_index(const QJsonArray& array) {
    QJsonObject result;
    for (int index = 0; index < array.size(); ++index) {
        auto id_array = array[index].toObject()["photo_ids"].toArray();
        for (QJsonValueRef id : id_array) {
            result[QString().setNum(id.toInt())] = index;
        }
    }
    return result;
}

void MainWindow::display(int index) {
    if (!ui->offline->isChecked()) {
        manager->get_image(records[index].links[pic_end_index]);
    } else {
        auto image = QImage(path() + records[index].pics[pic_end_index]);
//        auto image = QImage(QString(dir.path() + QDir::separator() + records[index].pics[pic_end_index]).chopped(3) + "jpg");
        ui->image->setPixmap(scaled(image));
    }
    disconnect(ui->text, &QTextEdit::textChanged, this, &MainWindow::set_edited);
    ui->text->setText(records[index].quote);
    connect(ui->text, &QTextEdit::textChanged, this, &MainWindow::set_edited);
    bool reached_end = index + 1 >= records.size();
    bool listing_on = pic_end_index + 1 < records[index].pics.size();
    ui->skip->setEnabled(config_edited);
    disconnect(ui->make_private, &QPushButton::toggled, this, &MainWindow::set_edited);
    ui->make_private->setChecked(!records[index].is_public);
    connect(ui->make_private, &QPushButton::toggled, this, &MainWindow::set_edited);
    ui->add->setEnabled(listing_on);
    ui->ok->setEnabled(!reached_end);
    ui->back->setEnabled(index > 0);
}

void MainWindow::draw(int index = 0) {
    if (!ui->offline->isChecked()) {
        manager->get_image(links[index]);
    } else {
        auto image = QImage(dir.path() + QDir::separator() + pics[index]);
        ui->image->setPixmap(scaled(image));
    }
    bool reached_end = index + 1 >= pics.size();
    ui->skip->setEnabled(!reached_end);
    ui->add->setEnabled(!reached_end);
    ui->back->setEnabled(index > 0);
}

void MainWindow::show_text(int index) {
    if (quotes.size() <= index) return;
    ui->text->setText(quotes[index]);
}
