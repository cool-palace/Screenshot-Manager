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

bool MainWindow::update_quote_file(const QString& title) {
    QFile file(quotes_location + title + ".txt");
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

bool MainWindow::update_quote_file(int start, int end) {
    QFile file(quotes_location + title_map.value(start) + ".txt");
    if (!file.open(QIODevice::WriteOnly | QIODevice::Truncate)) {
        ui->statusBar->showMessage("Не удалось открыть файл с цитатами.");
        return false;
    }
    QTextStream out(&file);
    out.setCodec("UTF-8");
    for (int i = start; i <= end; ++i) {
        out << records[i].quote + "\r\n";
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
    record.is_public = !ui->private_switch->isChecked();
    records.push_back(record);
}

void MainWindow::update_record() {
    auto text = ui->text->toPlainText();
    records[pic_index].quote = text;
    records[pic_index].is_public = !ui->private_switch->isChecked();
    record_items[pic_index]->update_text(text);
    record_edited = false;
    update_hashtag_info();
}

bool MainWindow::open_title_config(bool all) {
    clear_all();
    QStringList filepaths;
    if (!all) {
        filepaths = QFileDialog::getOpenFileNames(nullptr, "Открыть конфигурационный файл",
                                                           configs_location,
                                                           "Файлы (*.json)");
    } else {
        QDir dir = QDir(configs_location);
        dir.setNameFilters(QStringList("*.json"));
        filepaths = dir.entryList(QDir::Files | QDir::NoDotAndDotDot);
        if (filepaths.contains(".test.json")) {
            filepaths.removeAt(filepaths.indexOf(".test.json"));
        }
        for (QString& path : filepaths) {
            path = configs_location + path;
        }
    }
    if (filepaths.isEmpty()) return false;
    for (const auto& filepath : filepaths) {
        auto json_file = json_object(filepath);
        if (!json_file.contains("title") || !json_file.contains("screens")) {
            ui->statusBar->showMessage("Неверный формат файла: " + filepath);
            return false;
        }
        read_title_config(json_file);
    }
    return !records.empty();
}

bool MainWindow::open_public_config() {
    auto json_file = json_object(configs_location + "result\\public_records.json");
    if (!json_file.contains("records")) {
        ui->statusBar->showMessage("Неверный формат файла public_records.json" );
        return false;
    }
    auto records_array = json_file.value("records").toArray();
    for (QJsonValueRef r : records_array) {
        Record record;
        auto object = r.toObject();
        record.quote = object["caption"].toString(); // -preprocessed
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
    auto log = json_object(logs_location);
    for (auto key : log.keys()) {
        logs[key.toInt()] = log.value(key).toInt();
    }
    auto titles = json_file.value("title_map").toObject();
    for (auto index : titles.keys()) {
        title_map[index.toInt()] = titles[index].toString();
    }
    for (int i = records.size() - records_array.size(); i < records.size(); ++i) {
        record_items.push_back(new RecordItem(records[i], i, path(i)));
        if (logs.contains(records[i].ids[0])) {
            dynamic_cast<RecordItem*>(record_items.back())->include_log_info(logs.value(records[i].ids[0]));
        }

        ui->view_grid->addWidget(record_items.back());
        connect(record_items[i], &RecordItem::selected, [this](int index){
            selected_records[pic_index]->set_index(index);
            set_view(PREVIEW);
        });
    }
    return !records.empty();
}

void MainWindow::read_title_config(const QJsonObject& json_file) {
    auto title = json_file.value("title").toString();
    dir = QDir(screenshots_location + title);
    title_map[records.size()] = title;
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
    for (int i = records.size() - records_array.size(); i < records.size(); ++i) {
        record_items.push_back(new RecordItem(records[i], i, path(i)));
        connect(record_items[i], &RecordItem::selected, [this](int index){
            ui->slider->setValue(index);
            set_view(MAIN);
        });
    }
}

void MainWindow::save_title_config(const QString& title) {
    QJsonArray record_array;
    for (const auto& record : records) {
        record_array.push_back(record.to_json());
    }
    QFile file(configs_location + title + ".json");
    QJsonObject object;
    object["title"] = title;
    object["album_id"] = album_ids[title];
    object["screens"] = record_array;
    auto message = save_json(object, file)
            ? "Конфигурационный файл сохранён."
            : "Не удалось сохранить файл.";
    ui->statusBar->showMessage(message);
}

void MainWindow::save_title_config(int start, int end) {
    QJsonArray record_array;
    for (int i = start; i <= end; ++i) {
        record_array.push_back(records[i].to_json());
    }
    auto title = title_map.value(start);
    QFile file(configs_location + title + ".json");
    QJsonObject object;
    object["title"] = title;
    object["album_id"] = album_ids[title];
    object["screens"] = record_array;
    auto message = save_json(object, file)
            ? "Конфигурационный файл сохранён."
            : "Не удалось сохранить файл.";
    ui->statusBar->showMessage(message);
}

void MainWindow::read_text_from_subs() {
    dir = QDir(QFileDialog::getExistingDirectory(nullptr, "Открыть папку с кадрами",
                                                           screenshots_location));
    pics = dir.entryList(QDir::Files | QDir::NoDotAndDotDot);
    auto timestamps_for_filenames = timestamps_multimap();
    if (timestamps_for_filenames.isEmpty()) return;
    find_lines_by_timestamps(timestamps_for_filenames);
    update_quote_file(dir.dirName());
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

bool MainWindow::find_lines_by_timestamps(const QMultiMap<QString, QTime>& timestamps_for_filenames) {
//    auto dir_subs = QDir(subs_location);
//    auto dir_subs = QDir(QFileDialog::getExistingDirectory(nullptr, "Открыть папку с cубтитрами",
//                                                           screenshots_location));
    // The following lines are to be used when pic names and keys in multimap are sorted differently
//    auto keys = timestamps_for_filenames.uniqueKeys();
//    QList<QString> first, second;
//    for (auto&& key : keys) {
//        (key.startsWith('[') ? first : second).append(key);
//    }
//    auto filenames = first + second;
    for (const auto& filename : timestamps_for_filenames.uniqueKeys()) {
        auto path = QDir::toNativeSeparators(subs_location) + QDir::separator() + dir.dirName() + QDir::separator() + filename + ".ass";
        QFile file(path);
        if (!file.open(QIODevice::ReadOnly)) {
            QString message = "Ошибка при чтении субтитров: не удалось открыть файл " + file.fileName();
             ui->statusBar->showMessage(message);
            return false;
        }
        auto timestamps = timestamps_for_filenames.values(filename);
        QTextStream in(&file);
        QRegularExpression regex("Dialogue: \\d+,(\\d:\\d\\d:\\d\\d\\.\\d\\d),(\\d:\\d\\d:\\d\\d\\.\\d\\d),.+,0+,0+,0+,,({.+?})?(.+)");
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
                bool time_missed = time < line_start && time.addSecs(30) > line_start;
                if (time_within_bounds || time_missed) {
                    quotes.append(time_within_bounds ? match.captured(4).replace("\\N", " ") : last_line);
//                    records.append(Record(time_within_bounds ? match.captured(3) : last_line));
                    timestamps.pop_back();
                }
                last_line = match.captured(4).replace("\\N", " ");
            }
        }
        if (in.atEnd() && !timestamps.isEmpty()) {
            quotes.append("// Пропуск //");
        }
        file.close();
    }
    return true;
}

bool MainWindow::get_subs_for_pic() {
    QString filename;
    {
        QRegularExpression regex("(.*)?-(\\d-\\d\\d-\\d\\d-\\d{3})");
        auto i = regex.globalMatch(pics[pic_index]);
        if (i.hasNext()) {
            auto match = i.next();
            filename = match.captured(1);
        } else return false;
    }
    auto path = QDir::toNativeSeparators(subs_location) + QDir::separator() + dir.dirName() + QDir::separator() + filename + ".ass";
    QFile file(path);
    if (!file.open(QIODevice::ReadOnly)) {
        QString message = "Ошибка при чтении субтитров: не удалось открыть файл " + file.fileName();
         ui->statusBar->showMessage(message);
        return false;
    }
    QTextStream in(&file);
    QRegularExpression regex("Dialogue: \\d+,(\\d:\\d\\d:\\d\\d\\.\\d\\d),(\\d:\\d\\d:\\d\\d\\.\\d\\d),.+,0+,0+,0+,,({.+?})?(.+)");
    while (!in.atEnd()) {
        auto line = in.readLine();
        auto i = regex.globalMatch(line);
        if (i.hasNext()) {
            auto match = i.next();
            subs.append(match.captured(4).replace("\\N", " "));
        }
    }
    file.close();
    return true;
}

void MainWindow::compile_configs() {
    QDir dir = QDir(configs_location);
    auto configs = dir.entryList(QDir::Files | QDir::NoDotAndDotDot);
    if (configs.contains(".test.json")) {
        configs.removeAt(configs.indexOf(".test.json"));
    }
    QJsonArray resulting_array;
    QJsonArray hidden_array;
    QJsonObject title_map;
    for (const auto& config : configs) {
        auto object = json_object(configs_location + config);
        auto title = object["title"].toString();
        title_map[QString().setNum(resulting_array.size())] = title;
        auto array = object["screens"].toArray();
        for (QJsonValueRef item : array) {
            auto record = item.toObject();
            record["index"] = (record["public"].toBool() ? resulting_array : hidden_array).size();
            (record["public"].toBool() ? resulting_array : hidden_array).push_back(record);
        }
    }
    QJsonObject result, hidden_result;
    result["records"] = resulting_array;
    result["reverse_index"] = reverse_index(resulting_array);
    result["title_map"] = title_map;
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

void MainWindow::export_text() {
    QFile file("exported_text.txt");
    if (!file.open(QIODevice::WriteOnly | QIODevice::Truncate)) {
        ui->statusBar->showMessage("Не удалось открыть выходной файл.");
        return;
    }
    QRegularExpression regex("(.*?)\\s[#&].*$");
    QTextStream out(&file);
    out.setCodec("UTF-8");
    QDir dir = QDir(configs_location);
    auto configs = dir.entryList(QDir::Files | QDir::NoDotAndDotDot);
    for (const auto& config : configs) {
//        if (config.startsWith("Kono")) break;
        auto object = json_object(configs_location + config);
        auto array = object["screens"].toArray();
        for (QJsonValueRef item : array) {
            auto record = item.toObject();
            if (record["public"].toBool()) {
                auto quote = record["caption"].toString();
//                auto i = regex.globalMatch(quote);
//                if (i.hasNext()) {
//                    auto match = i.next();
//                    quote = match.captured(1);
//                }
                out << quote + "\r\n";
            }
        }
    }
    file.close();
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
        auto image = QImage(path(index) + records[index].pics[pic_end_index]);
        if (image.isNull()) image = QImage(QString(path(index) + records[index].pics[pic_end_index]).chopped(3) + "jpg");
        ui->image->setPixmap(scaled(image));
    }
    disconnect(ui->text, &QTextEdit::textChanged, this, &MainWindow::set_edited);
    ui->text->setText(records[index].quote);
    connect(ui->text, &QTextEdit::textChanged, this, &MainWindow::set_edited);
    bool reached_end = index + 1 >= records.size();
    bool listing_on = pic_end_index + 1 < records[index].pics.size();
    ui->save->setEnabled(!edited_ranges.empty());
    disconnect(ui->private_switch, &QAction::triggered, this, &MainWindow::set_edited);
    ui->private_switch->setChecked(!records[index].is_public);
    connect(ui->private_switch, &QAction::triggered, this, &MainWindow::set_edited);
    ui->add->setEnabled(listing_on);
    ui->ok->setEnabled(!reached_end);
    ui->back->setEnabled(index > 0);
}

void MainWindow::draw(int index = 0) {
    if (!ui->offline->isChecked() && current_mode != TEXT_READING) {
        manager->get_image(links[index]);
    } else {
        auto image = QImage(dir.path() + QDir::separator() + pics[index]);
        ui->image->setPixmap(scaled(image));
    }
    bool reached_end = index + 1 >= pics.size();
    ui->skip->setEnabled(!reached_end && current_mode != TEXT_READING);
    ui->add->setEnabled(!reached_end && current_mode != TEXT_READING);
    ui->back->setEnabled(index > 0);
}

void MainWindow::show_text(int index) {
    if (quotes.size() <= index && subs.empty()) return;
    bool subtitles_on = !subs.empty();
    ui->text->setText(subtitles_on ? subs[index] : quotes[index]);
}

QString MainWindow::attachments(int index) const {
    QString result;
    for (const auto& photo_id : records[index].ids) {
        if (!result.isEmpty()) result += ",";
        result += prefix + QString().setNum(photo_id);
    }
    return result;
}
