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
    set_mode(data_ready() ? CONFIG_CREATION : IDLE);
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
    ui->statusBar->showMessage(QString("Опрос опубликован"));
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
    if (!open_title_config()) {
        set_mode(IDLE);
        ui->statusBar->showMessage("Конфигурационный файл не открыт.");
        return;
    }
    set_mode(CONFIG_READING);
    if (current_view == PREVIEW) set_view(MAIN);
}

void MainWindow::journal_reading_all() {
    if (!open_title_config(true)) {
        set_mode(IDLE);
        ui->statusBar->showMessage("Конфигурационный файл не открыт.");
        return;
    }
    set_mode(CONFIG_READING);
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

void MainWindow::release_preparation() {
    clear_all();
    if (!open_public_config()) {
        set_mode(IDLE);
        ui->statusBar->showMessage("Конфигурационный файл не открыт.");
        return;
    }
    set_mode(RELEASE_PREPARATION);
    set_view(PREVIEW);
}

void MainWindow::poll_preparation() {
    bool poll_mode = ui->poll_preparation->isChecked();
    QLayoutItem* child;
    while ((child = ui->preview_grid->takeAt(0))) {
        // Clearing items from the grid
        child->widget()->hide();
    }
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
    case CONFIG_READING:
        pic_end_index = 0;
        pic_index = value;
        display(pic_index);
        update_current_hashtags();
        break;
    case TEXT_READING:
        quote_index = value;
        show_text(value);
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
    QLayoutItem* child;
    while ((child = ui->preview_grid->takeAt(0))) {
        // Clearing items from the grid
        child->widget()->hide();
    }
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
        connect(selected_records.back(), &RecordPreview::reroll_request, [this](int selected_index){
            connect(this, &MainWindow::reroll_response, selected_records[selected_index], &RecordPreview::set_index);
            emit reroll_response(random_index());
            disconnect(this, &MainWindow::reroll_response, selected_records[selected_index], &RecordPreview::set_index);
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
        int r_index = QRandomGenerator::global()->bounded(full_hashtags.size());
        auto tag = full_hashtags[r_index];
        if (!selected_hashtags.contains(tag.tag())) {
            selected_hashtags[tag.tag()] = new HashtagPreview(tag);
            connect(selected_hashtags[tag.tag()], &HashtagPreview::reroll_request, [this](const QString& old_tag){
                auto preview = selected_hashtags[old_tag];
                selected_hashtags.remove(old_tag);
                Hashtag tag;
                do {
                    int index = QRandomGenerator::global()->bounded(full_hashtags.size());
                    tag = full_hashtags[index];
                    qDebug() << tag.tag() << selected_hashtags.contains(tag.tag());
                } while (selected_hashtags.contains(tag.tag()));
                selected_hashtags.insert(tag.tag(), preview);
                selected_hashtags[tag.tag()]->set_hashtag(tag);
                QLayoutItem* child;
                while ((child = ui->preview_grid->takeAt(0))) {
                    // Clearing items from the grid
                }
                for (auto item : selected_hashtags) {
                    ui->preview_grid->addWidget(item);
                }
            });
        }
    }
    for (const auto& tag : selected_hashtags) {
        ui->preview_grid->addWidget(tag);
    }
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
        qDebug() << poll_end;
        manager->get_poll(options(), poll_end.toSecsSinceEpoch());
    }
}

void MainWindow::skip_button() {
    switch (current_mode) {
    case CONFIG_CREATION:
        pic_index = qMax(pic_index, pic_end_index) + 1;
        draw(pic_index);
        ui->slider->setValue(pic_index);
        show_status();
        break;
    case CONFIG_READING:
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
    case CONFIG_CREATION:
        // Adding one more image to current record
        pic_end_index = qMax(pic_index, pic_end_index) + 1;
        draw(pic_end_index);
        ui->slider->setValue(pic_end_index);
        break;
    case CONFIG_READING:
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
    case CONFIG_CREATION:
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
    case CONFIG_READING:
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
    case CONFIG_CREATION:
        register_record();
        pic_index = qMax(pic_index, pic_end_index) + 1;
        pic_end_index = 0;
        ui->private_switch->setChecked(false);
        if (pic_index < pics.size()) {
            draw(pic_index);
            ui->slider->setValue(pic_index);
            show_text(++quote_index);
        } else {
            save_title_config(dir.dirName());
            update_quote_file(dir.dirName());
            set_mode(IDLE);
        }
        break;
    case CONFIG_READING:
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
    default:
        break;
    }
    show_status();
}