#include "include\text_reading.h"

TextReading::TextReading(MainWindow* parent) : AbstractPreparationMode(parent) {
    connect(ui->load_subs, &QAction::triggered, this, &TextReading::load_subs);
    QDir dir = QDir(QFileDialog::getExistingDirectory(nullptr, "Открыть папку с кадрами",
                                                 Locations::instance()[SCREENSHOTS]));
    pics = dir.entryList(QDir::Files | QDir::NoDotAndDotDot);
    title_map.insert(0, dir.dirName());
}

TextReading::~TextReading() {
    disconnect(ui->load_subs, nullptr, this, nullptr);
    set_enabled(false);
}

void TextReading::start() {
    set_view(MAIN);
    if (data_ready()) {
        ui->ok->setText("Готово");
        ui->skip->setText("Предыдущий");
        ui->add->setText("Следующий");
        ui->stackedWidget->setCurrentIndex(0);
        draw(pic_index);
        show_text(pic_index);
        set_enabled(true);
        show_status();
    } else set_enabled(false);
}

void TextReading::slider_change(int value) {
    quote_index = value;
    show_text(value);
    show_status();
}

void TextReading::skip_button() {
    if (quote_index > 0) {
        ui->slider->setValue(--quote_index);
    }
    show_status();
}

void TextReading::add_button() {
    if (quote_index < subs.size() - 1) {
        ui->slider->setValue(++quote_index);
    }
    show_status();
}

void TextReading::back_button() {
    if (pic_index == 0) return;
    clear_subs();
    draw(--pic_index);
    show_text(pic_index);
    show_status();
}

void TextReading::ok_button() {
    if (!subs.isEmpty()) {
        quotes[pic_index] = ui->text->toPlainText();
        clear_subs();
    }
    ++pic_index;
    if (pic_index < pics.size()) {
        draw(pic_index);
        show_text(pic_index);
        show_status();
    } else {
        set_enabled(false);
        update_quote_file();
    }
}

void TextReading::set_enabled(bool enable) {
    ui->main_view->setEnabled(enable);
    ui->back->setEnabled(enable && pic_index > 0);
    ui->ok->setEnabled(enable);
    ui->load_subs->setEnabled(enable);
    ui->slider->setValue(0);
    ui->add->setEnabled(false);
    ui->skip->setEnabled(false);
    ui->list_view->setEnabled(false);
}

void TextReading::lay_previews(int page) {
    if (current_view == MAIN) return;
    int pics_per_page = ui->pics_per_page->value();
    int total_previews = record_items.size();
    ui->page_index->setMaximum(total_previews / pics_per_page + 1);
    clear_grid(ui->view_grid);
    for (int i = (page - 1) * pics_per_page ; i < qMin(record_items.size(), page * pics_per_page); ++i) {
        if (current_view == LIST) {
            record_items[i]->set_list_view();
            ui->view_grid->addWidget(record_items[i], i, 0);
        } else {
            record_items[i]->set_gallery_view();
            ui->view_grid->addWidget(record_items[i], i/10, i%10);
        }
    }
}

void TextReading::draw(int index = 0) {
    auto dir_path = Locations::instance()[SCREENSHOTS] + title_name() + QDir::separator();
    auto image = QImage(dir_path + pics[index]);
    if (image.isNull()) {
        // Checking the reserve screenshot folder for text reading
        image = QImage(Locations::instance()[SCREENSHOTS_NEW] + title_name() + QDir::separator() + pics[index]);
    }
    ui->image->setPixmap(scaled(image));
    ui->back->setEnabled(index > 0);
}

void TextReading::show_text(int index) {
    if (quotes.size() <= index && subs.empty()) return;
    bool subtitles_on = !subs.empty();
    ui->text->setText(subtitles_on ? subs[index] : quotes[index]);
    ui->text->setAlignment(Qt::AlignHCenter);
}

void TextReading::show_status() {
    QString s_line = subs.isEmpty()
            ? ""
            : ", строка " + QString().setNum(quote_index + 1) + " из " + QString().setNum(subs.size());;
    QString s_pic = QString().setNum(pic_index + 1) + " из " + QString().setNum(pics.size());
    ui->statusBar->showMessage("Кадр " + s_pic + s_line);
}

bool TextReading::data_ready() {
    if (pics.empty()) {
        ui->statusBar->showMessage("Выбранная папка не содержит кадров.");
        return false;
    }
    auto timestamps_for_filenames = timestamps_multimap();
    if (timestamps_for_filenames.isEmpty()) {
        ui->statusBar->showMessage("Не удалось извлечь временные метки из кадров.");
        return false;
    }
    return find_lines_by_timestamps(timestamps_for_filenames);
}

QMultiMap<QString, QTime> TextReading::timestamps_multimap() {
    QMultiMap<QString, QTime> timestamps_for_filenames;
    int ms_offset = -250;
    for (const auto& pic : pics) {
        auto i = timestamp_regex.globalMatch(pic);
        if (i.hasNext()) {
            auto match = i.next();
            auto filename = match.captured(1);
            auto time = QTime::fromString(match.captured(2), "h-mm-ss-zzz").addMSecs(ms_offset);
            timestamps_for_filenames.insert(filename, time);
        }
    }
    return timestamps_for_filenames;
}

void TextReading::generate_combinations(const QString& input, int index, QString current, QStringList& results) const {
    if (index == input.length()) {
        results.push_back(current);
        return;
    }
    if (input[index] == '_') {
        // Заменяем "_" на "." и добавляем в комбинации
        generate_combinations(input, index + 1, current + '.', results);
    }
    // Оставляем "_" как есть
    generate_combinations(input, index + 1, current + input[index], results);
}

QString TextReading::subs_filename(const QString& filename, const QString& title) const {
    QStringList combinations;
    generate_combinations(filename, 0, "", combinations);
    qDebug() << combinations;
    for (const QString& combination : combinations) {
        QString subs_path = QDir::toNativeSeparators(Locations::instance()[SUBS]) + title + QDir::separator() + combination + ".ass";
        if (QFile::exists(subs_path)) {
            return subs_path;
        } else qDebug() << "Subs file does not exist: " << subs_path;
    }
    return QString();
}

bool TextReading::find_lines_by_timestamps(const QMultiMap<QString, QTime>& timestamps_for_filenames) {
    // The following lines are to be used when pic names and keys in multimap are sorted differently
//    auto keys = timestamps_for_filenames.uniqueKeys();
//    QList<QString> first, second;
//    for (auto&& key : keys) {
//        (key.startsWith('[') ? first : second).append(key);
//    }
//    auto filenames = first + second;
    for (const auto& filename : timestamps_for_filenames.uniqueKeys()) {
        auto path = subs_filename(filename, title_name());
        QFile file(path);
        if (!file.open(QIODevice::ReadOnly)) {
            QString message = "Ошибка при чтении субтитров: не удалось открыть файл " + file.fileName();
             ui->statusBar->showMessage(message);
            return false;
        }
        auto timestamps = timestamps_for_filenames.values(filename);
        QTextStream in(&file);
        QString last_line;
        while (!timestamps.isEmpty() && !in.atEnd()) {
            auto line = in.readLine();
            auto i = line_regex.globalMatch(line);
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

bool TextReading::get_subs_for_pic() {
    QString filename;
    {
        auto i = timestamp_regex.globalMatch(pics[pic_index]);
        if (i.hasNext()) {
            auto match = i.next();
            filename = match.captured(1);
        } else return false;
    }
    auto path = subs_filename(filename, title_name());
    QFile file(path);
    if (!file.open(QIODevice::ReadOnly)) {
        QString message = "Ошибка при чтении субтитров: не удалось открыть файл " + file.fileName();
        ui->statusBar->showMessage(message);
        return false;
    }
    QTextStream in(&file);
    while (!in.atEnd()) {
        auto line = in.readLine();
        auto i = line_regex.globalMatch(line);
        if (i.hasNext()) {
            auto match = i.next();
            subs.append(match.captured(4).replace("\\N", " "));
        }
    }
    file.close();
    return true;
}

void TextReading::load_subs() {
    ui->load_subs->setDisabled(true);
    if (get_subs_for_pic()) {
        ui->add->setEnabled(true);
        ui->skip->setEnabled(true);
        ui->list_view->setEnabled(true);
        ui->page_index->setEnabled(true);
        ui->pics_per_page->setEnabled(true);
        quote_index = subs.indexOf(ui->text->toPlainText());
        ui->slider->setEnabled(true);
        ui->slider->setMaximum(subs.size() - 1);
        ui->slider->setValue(quote_index);
        ui->page_index->setMaximum(subs.size() / ui->pics_per_page->value() + 1);
        QMap<QString, int> lines;
        for (int i = 0; i < subs.size(); ++i) {
            lines.insert(subs[i], i);
        }
        for (const auto& line : lines.keys()) {
            record_items.push_back(new RecordItem(line, lines[line]));
            connect(record_items.back(), &RecordItem::selected, [this](int index){
                ui->slider->setValue(index);
                set_view(MAIN);
            });
            ui->view_grid->addWidget(record_items.back());
        }
    }
}

void TextReading::clear_subs() {
    subs.clear();
    for (auto item : record_items) {
        delete item;
    }
    record_items.clear();
    ui->load_subs->setEnabled(true);
    ui->slider->setEnabled(false);
    ui->skip->setEnabled(false);
    ui->add->setEnabled(false);
    ui->list_view->setEnabled(false);
    ui->page_index->setEnabled(false);
    ui->pics_per_page->setEnabled(false);
}

void TextReading::update_quote_file() {
    for (const auto& quote : quotes) {
        records.append(Record(quote));
    }
    if (AbstractPreparationMode::update_quote_file(title_name())) {
        ui->statusBar->showMessage("Цитаты записаны в файл " + title_name() + ".txt");
    } else ui->statusBar->showMessage("Не удалось записать файл " + title_name() + ".txt");
}
