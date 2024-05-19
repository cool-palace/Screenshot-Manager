#include "mainwindow.h"
#include "ui_mainwindow.h"
#include <QDebug>

void MainWindow::set_albums(const QMap<QString, int>& ids) {
    album_ids = ids;
    qDebug() << ids;
    if (album_ids.empty()) {
        ui->offline->setChecked(true);
        ui->statusBar->showMessage("Не удалось загрузить альбомы. Попробуйте авторизироваться вручную или продолжите работу оффлайн.");
    }
}

void MainWindow::set_photo_ids(const QVector<int>& ids, const QStringList& urls) {
    photo_ids = ids;
    links = urls;
    set_mode(data_ready() ? JOURNAL_CREATION : IDLE);
}

void MainWindow::set_loaded_image(const QImage& image) {
    ui->image->setPixmap(scaled(image));
}

void MainWindow::posting_success(int index, int date) {
    status_mutex.lock();
    auto current_status = ui->statusBar->currentMessage();
    if (current_status.startsWith("Опубликованы записи")) {
        current_status.append(QString(", %1").arg(index+1));
        ui->statusBar->showMessage(current_status);
    } else {
        ui->statusBar->showMessage(QString("Опубликованы записи: %1").arg(index+1));
    }
    for (const int photo_id : records[index].ids) {
        logs[photo_id] = date;
    }
    if (--post_counter == 0) {
        update_logs();
    }
    status_mutex.unlock();
}

void MainWindow::posting_fail(int index, const QString& reply) {
    if (post_counter < selected_records.size()) {
        update_logs();
        post_counter = 0;
    }
    ui->statusBar->showMessage(QString("Не удалось опубликовать запись %1").arg(index+1));
    QMessageBox msgBox(QMessageBox::Critical, "Ошибка", reply);
    msgBox.exec();
}

void MainWindow::post_poll(int id) {
    int time = QDateTime(ui->date->date(), ui->time->time(), Qt::LocalTime).toSecsSinceEpoch();
    manager->post(poll_message(), id, time);
}

void MainWindow::poll_posting_success() {
    int time = QDateTime(ui->date->date(), ui->time->time(), Qt::LocalTime).toSecsSinceEpoch();
    for (const auto& tag : selected_hashtags.keys()) {
        poll_logs[tag] = time;
        if (selected_hashtags[tag]->is_edited()) {
            qDebug() << selected_hashtags[tag]->text_description();
            full_hashtags_map[tag].set_description(selected_hashtags[tag]->text_description());

        }
    }
    update_hashtag_file();
    update_poll_logs();
    auto current_message = ui->statusBar->currentMessage();
    ui->statusBar->showMessage(current_message + ". Опрос опубликован");
}

void MainWindow::poll_posting_fail(const QString& reply) {
    ui->statusBar->showMessage(QString("Не удалось опубликовать опрос"));
    QMessageBox msgBox(QMessageBox::Critical, "Ошибка", reply);
    msgBox.exec();
}

void MainWindow::caption_success() {
    captions_for_ids.remove(captions_for_ids.firstKey());
    if (!captions_for_ids.empty()) {
        QThread::msleep(350);
        add_caption();
    } else {
        ui->statusBar->showMessage("Добавление подписей прошло успешно.");
    }
}

void MainWindow::captcha_handling(const QString& captcha_id) {
    ui->statusBar->showMessage(QString("Осталось подписать %1 фотографий").arg(captions_for_ids.size()));
    bool ok;
    QString text = QInputDialog::getText(this, tr("Капча"),
                                               tr("Введите капчу:"), QLineEdit::Normal,
                                               "", &ok);
    if (ok && !text.isEmpty()) {
        add_caption(captcha_id, text);
    }
    for (int i = 0; i < records.size(); ++i) {
        if (records[i].ids.contains(captions_for_ids.firstKey())) {
            display(i);
            break;
        }
    }
}

void MainWindow::journal_creation() {
    clear_all();
    dir = QDir(QFileDialog::getExistingDirectory(nullptr, "Открыть папку с кадрами",
                                                 locations[SCREENSHOTS]));
    pics = dir.entryList(QDir::Files | QDir::NoDotAndDotDot);
    QFile file(locations[QUOTES] + dir.dirName() + ".txt");
    if (!read_quote_file(file)) {
        clear_all();
        return;
    }
    manager->get_photo_ids(album_ids[dir.dirName()]);
}

void MainWindow::journal_reading() {
    if (!open_title_journal()) {
        set_mode(IDLE);
        ui->statusBar->showMessage("Конфигурационный файл не открыт.");
        return;
    }
    set_mode(JOURNAL_READING);
    if (current_view == PREVIEW) set_view(MAIN);
}

void MainWindow::journal_reading_all() {
    if (!open_title_journal(true)) {
        set_mode(IDLE);
        ui->statusBar->showMessage("Конфигурационный файл не открыт.");
        return;
    }
    set_mode(JOURNAL_READING);
    if (current_view == PREVIEW) set_view(MAIN);
}

void MainWindow::text_reading() {
    clear_all();
    dir = QDir(QFileDialog::getExistingDirectory(nullptr, "Открыть папку с кадрами",
                                                 locations[SCREENSHOTS]));
    pics = dir.entryList(QDir::Files | QDir::NoDotAndDotDot);
    auto timestamps_for_filenames = timestamps_multimap();
    if (timestamps_for_filenames.isEmpty()) {
        set_mode(IDLE);
        ui->statusBar->showMessage("Не удалось извлечь временные метки из кадров.");
        return;
    }
    if (find_lines_by_timestamps(timestamps_for_filenames)) {
        set_mode(TEXT_READING);
    }
}

void MainWindow::descriptions_reading() {
    clear_all();
    auto path = QFileDialog::getOpenFileName(nullptr, "Открыть файл с описаниями",
                                                       locations[JOURNALS] + "//descriptions",
                                                       "Файлы (*.json)");
    read_descriptions(json_object(path));
    if (records.isEmpty()/* quotes.isEmpty() || pics.isEmpty()*/) {
        set_mode(IDLE);
        ui->statusBar->showMessage("Не удалось прочесть описания из файла.");
        return;
    }
    set_mode(DESCRIPTION_READING);
    set_view(MAIN);
}

void MainWindow::release_preparation() {
    clear_all();
    if (!open_public_journal()) {
        set_mode(IDLE);
        ui->statusBar->showMessage("Конфигурационный файл не открыт.");
        return;
    }
    set_mode(RELEASE_PREPARATION);
    set_view(PREVIEW);
}

void MainWindow::poll_preparation() {
    bool poll_mode = ui->poll_preparation->isChecked();
    clear_grid(ui->preview_grid);
    if (poll_mode) for (auto tag : selected_hashtags) {
        ui->preview_grid->addWidget(tag);
        tag->show();
    } else for (auto record : selected_records) {
        ui->preview_grid->addWidget(record);
        record->show();
    }
    ui->time->setTime(poll_mode ? QTime(12,5) : QTime(8,0));
    ui->quantity->setValue(poll_mode ? 6 : 7);
    int day = ui->date->date().dayOfWeek();
    int hours = (4 - day) * 24 + 9;      // Set to end on thursday evening
    // Interval hh : mm are used as days : hours in poll mode
    ui->interval->setTime(poll_mode && hours > 0 ? QTime(hours/24, hours%24) : QTime(2,0));
}

void MainWindow::show_public(bool checked) {
    if (checked && ui->show_private->isChecked()) {
        ui->show_private->setChecked(false);
    }
    filter_event(true);
}

void MainWindow::show_private(bool checked) {
    if (checked && ui->show_public->isChecked()) {
        ui->show_public->setChecked(false);
    }
    filter_event(false);
}

void MainWindow::slider_change(int value) {
    switch (current_mode) {
    case JOURNAL_READING:
        pic_end_index = 0;
        pic_index = value;
        display(pic_index);
        update_current_hashtags();
        break;
    case TEXT_READING:
        quote_index = value;
        show_text(value);
        break;
    case DESCRIPTION_READING:
        pic_index = value;
        display(pic_index);
        break;
    default:
        break;
    }
    show_status();
}

void MainWindow::load_subs() {
    ui->load_subs->setDisabled(true);
    if (get_subs_for_pic()) {
        ui->add->setEnabled(true);
        ui->skip->setEnabled(true);
        quote_index = subs.indexOf(ui->text->toPlainText());
        ui->slider->setEnabled(true);
        ui->slider->setMaximum(subs.size() - 1);
        ui->slider->setValue(quote_index);
        ui->page_index->setMaximum(subs.size() / pics_per_page + 1);
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
//            lay_previews();
    }
}

void MainWindow::add_hashtag() {
    bool ok;
    QString text = QInputDialog::getText(this, tr("Добавление хэштега"),
                                               tr("Введите новый хэштег:"), QLineEdit::Normal,
                                               "", &ok);
    if (ok && !text.isEmpty() && !hashtags.contains(text)) {
        create_hashtag_button(text);
        update_hashtag_grid();
    }
}

void MainWindow::generate_button() {
    clear_grid(ui->preview_grid);
    ui->poll_preparation->isChecked() ? generate_poll() : generate_release();
}

void MainWindow::generate_release() {
    for (auto record : selected_records) {
        delete record;
    }
    selected_records.clear();
    QDateTime time = QDateTime(ui->date->date(), ui->time->time(), Qt::LocalTime);
    for (int i = 0; i < ui->quantity->value(); ++i) {
        int r_index = random_index();
        selected_records.push_back(new RecordPreview(records[r_index], r_index, time));
        connect(selected_records.back(), &RecordPreview::search_start, [this](int index){
            pic_index = index;
            set_view(LIST);
        });
        connect(selected_records.back(), &RecordPreview::reroll_request, [this](RecordPreview* preview){
            preview->set_index(random_index());
        });
        time = time.addSecs(ui->interval->time().hour()*3600 + ui->interval->time().minute()*60);
        ui->preview_grid->addWidget(selected_records.back());
        selected_records.back()->set_list_view();
    }
    RecordPreview::selected_records = &selected_records;
}

void MainWindow::generate_poll() {
    for (auto tag : selected_hashtags) {
        delete tag;
    }
    selected_hashtags.clear();
    while (selected_hashtags.size() < ui->quantity->value()) {
        int r_index = QRandomGenerator::global()->bounded(full_hashtags_map.keys().size());
        auto tag = full_hashtags_map.keys()[r_index];
        if (!selected_hashtags.contains(tag)) {
            selected_hashtags[tag] = new HashtagPreview();
            create_hashtag_preview_connections(tag);
            // Setting the actual tag contents
            selected_hashtags[tag]->set_hashtag(full_hashtags_map[tag]);
        }
    }
    for (const auto& tag : selected_hashtags) {
        ui->preview_grid->addWidget(tag);
    }
}

void MainWindow::check_logs() {
    filter_event(logs);
    lay_previews();
    set_view(LIST);
}

void MainWindow::create_hashtag_preview_connections(const QString& tag) {
    connect(selected_hashtags[tag], &HashtagPreview::reroll_request, [this](const QString& old_tag){
        // Saving the pointer to the hashtag preview to replace
        auto preview = selected_hashtags[old_tag];
        selected_hashtags.remove(old_tag);
        QString tag;
        do {
            int index = QRandomGenerator::global()->bounded(full_hashtags_map.keys().size());
            tag = full_hashtags_map.keys()[index];
            qDebug() << tag << selected_hashtags.contains(tag);
        } while (selected_hashtags.contains(tag));
        change_selected_hashtag(tag, preview);
    });
    connect(selected_hashtags[tag], &HashtagPreview::search_start, [this](const QString& old_tag){
        // Saving the pointer to the hashtag preview to replace
        auto preview = selected_hashtags[old_tag];
        selected_hashtags.remove(old_tag);
        HashtagButton::set_preview_to_change(preview);
        set_view(MAIN);
    });
    connect(selected_hashtags[tag], &HashtagPreview::count_request, [this](const QString& tag){
        // Saving the pointer to the hashtag preview to replace
        int count = hashtags[tag]->get_count();
        selected_hashtags[tag]->update_count(count);
    });
    connect(selected_hashtags[tag], &HashtagPreview::check_request, [this](const QString& tag){
        filters.clear();
        exit_filtering();
        emit hashtags[tag]->filterEvent(FilterType::ANY_TAG, tag);;
        set_view(LIST);
    });
}

void MainWindow::post_button() {
    if (!ui->poll_preparation->isChecked()) {
        post_counter = selected_records.size();
        for (auto record : selected_records) {
            int index = record->get_index();
            manager->post(index, attachments(index), record->timestamp());
        }
    } else {
        QDateTime poll_end = QDateTime(ui->date->date(), ui->time->time(), Qt::LocalTime);
        int time_offset = ui->interval->time().hour()*24*3600 + ui->interval->time().minute()*3600 - 300;
        poll_end = poll_end.addSecs(time_offset);
        manager->get_poll(options(), poll_end.toSecsSinceEpoch());
    }
}

void MainWindow::skip_button() {
    switch (current_mode) {
    case JOURNAL_CREATION:
        pic_index = qMax(pic_index, pic_end_index) + 1;
        draw(pic_index);
        ui->slider->setValue(pic_index);
        show_status();
        break;
    case JOURNAL_READING:
        // Saving button
            save_changes();
        break;
    case TEXT_READING:
        if (quote_index > 0) {
            ui->slider->setValue(--quote_index);
        }
        break;
    default:
        break;
    }
}

void MainWindow::add_button() {
    switch (current_mode) {
    case JOURNAL_CREATION:
        // Adding one more image to current record
        pic_end_index = qMax(pic_index, pic_end_index) + 1;
        draw(pic_end_index);
        ui->slider->setValue(pic_end_index);
        break;
    case JOURNAL_READING:
        // Showing next image in current record
        ++pic_end_index;
        display(pic_index);
        if (!filtration_results.isEmpty() && filtration_results.find(pic_index) == filtration_results.begin()) {
            ui->back->setDisabled(true);
        }
        break;
    case TEXT_READING:
        if (quote_index < subs.size() - 1) {
            ui->slider->setValue(++quote_index);
        }
        break;
    default:
        break;
    }
    show_status();
}

void MainWindow::back_button() {
    switch (current_mode) {
    case JOURNAL_CREATION:
        if (ui->back->text() == "Назад") {
            // Going back in picture list
            if (pic_index == pic_end_index) {
                pic_end_index = 0;
            }
        {
            int& current_index = pic_index > pic_end_index ? pic_index : pic_end_index;
            if (current_index == 0) break;
            draw(--current_index);
            ui->slider->setValue(current_index);
        }
        } else { // Cancelling the latest operation
            if (pic_end_index > 0) {
                // Pic was added by mistake
                pic_end_index = 0;
                draw(pic_index);
            } else {
                // A record was registered by mistake
                auto rec = records.takeLast();
                pic_index -= rec.pics.size();
                draw(pic_index);
                ui->slider->setValue(pic_index);
                show_text(--quote_index);
            }
        }
        break;
    case JOURNAL_READING: case DESCRIPTION_READING:
        pic_end_index = 0;
        if (ui->text->isEnabled()) {
            ui->slider->setValue(pic_index - 1);
        } else {
            auto it = --filtration_results.find(pic_index);
            ui->slider->setValue(it.key());
            ui->back->setEnabled(it != filtration_results.begin());
        }
        break;
    case TEXT_READING:
        if (pic_index == 0) break;
        subs.clear();
        for (auto item : record_items) {
            delete item;
        }
        record_items.clear();
        ui->load_subs->setEnabled(true);
        draw(--pic_index);
        show_text(pic_index);
        ui->slider->setEnabled(false);
//            ui->slider->setValue(pic_index);
        ui->skip->setEnabled(false);
        ui->add->setEnabled(false);
        break;
    default:
        break;
    }
    show_status();
}

void MainWindow::ok_button() {
    switch (current_mode) {
    case JOURNAL_CREATION:
        register_record();
        pic_index = qMax(pic_index, pic_end_index) + 1;
        pic_end_index = 0;
        ui->private_switch->setChecked(false);
        if (pic_index < pics.size()) {
            draw(pic_index);
            ui->slider->setValue(pic_index);
            show_text(++quote_index);
        } else {
            save_title_journal(dir.dirName());
            update_quote_file(dir.dirName());
            set_mode(IDLE);
        }
        break;
    case JOURNAL_READING: case DESCRIPTION_READING:
        if (record_edited) {
            update_record();
            if (pic_index + 1 == records.size()) {
                ui->ok->setEnabled(false);
                ui->save->setEnabled(true);
                break;
            }
        }
        pic_end_index = 0;
        if (ui->text->isEnabled()) {
            ui->slider->setValue(pic_index + 1);
        } else {
            auto it = ++filtration_results.find(pic_index);
            ui->slider->setValue(it.key());
            ui->ok->setEnabled(++it != filtration_results.end());
        }
        break;
    case TEXT_READING:
        if (!subs.isEmpty()) {
            quotes[pic_index] = ui->text->toPlainText();
            subs.clear();
            for (auto item : record_items) {
                delete item;
            }
            record_items.clear();
            ui->load_subs->setEnabled(true);
            ui->slider->setEnabled(false);
        }
        ++pic_index;
        if (pic_index < pics.size()) {
            draw(pic_index);
            show_text(pic_index);
//                ui->slider->setValue(pic_index);
        } else {
            for (const auto& quote : quotes) {
                records.append(Record(quote));
            }
            update_quote_file(dir.dirName());
            ui->statusBar->showMessage("Цитаты записаны в файл " + dir.dirName() + ".txt");
            set_mode(IDLE);
        }
        break;
//    case DESCRIPTION_READING:
//        if (record_edited) {
//            update_record();
//            if (pic_index + 1 == records.size()) {
//                ui->ok->setEnabled(false);
//                ui->save->setEnabled(true);
//                break;
//            }
//        }
//        ui->slider->setValue(pic_index + 1);
//            if (++pic_index < pics.size()) {
////                draw(pic_index);
////                show_text(++quote_index);
////                display(pic_index);
//                ui->slider->setValue(pic_index);
//            } else {
////                save_title_journal(dir.dirName());
////                update_quote_file(dir.dirName());
////                set_mode(IDLE);
//            }
        break;
    default:
        break;
    }
    show_status();
}
