#include "journal_reading.h"

JournalReading::JournalReading(MainWindow *parent, bool all) : AbstractOperationMode(parent), record_edited(all)
{
    connect(manager, &VK_Manager::caption_passed, this, &JournalReading::caption_success);
    connect(manager, &VK_Manager::captcha_error, this, &JournalReading::captcha_handling);
    connect(ui->save, &QAction::triggered, this, &JournalReading::save_changes);
    connect(ui->export_captions_by_ids, &QAction::triggered, this, &JournalReading::export_captions_by_ids);
    connect(ui->export_info_by_ids, &QAction::triggered, this, &JournalReading::export_info_by_ids);
    connect(ui->add_caption, &QAction::triggered, [this]() {
        for (int i = 0; i < records.size(); ++i) {
            QString title = title_captions[title_name(i)];
            for (int id : records[i].ids) {
                captions_for_ids[id] = title;
            }
        }
        add_caption();
    });
    connect(ui->show_public, &QAction::triggered, this, &JournalReading::show_public);
    connect(ui->show_private, &QAction::triggered, this, &JournalReading::show_private);
    connect(ui->private_switch, &QAction::triggered, this, &JournalReading::set_edited);
    connect(parent, &MainWindow::key_press, this, &JournalReading::keyPressEvent);
    connect(parent, &MainWindow::key_release, this, &JournalReading::keyReleaseEvent);
    connect(parent, &MainWindow::close_event, this, &JournalReading::closeEvent);
}

JournalReading::~JournalReading() {
    disconnect(ui->save, nullptr, this, nullptr);
    disconnect(ui->export_captions_by_ids, nullptr, this, nullptr);
    disconnect(ui->export_info_by_ids, nullptr, this, nullptr);
    disconnect(ui->add_caption, nullptr, this, nullptr);
    disconnect(ui->show_public, nullptr, this, nullptr);
    disconnect(ui->show_private, nullptr, this, nullptr);
    disconnect(ui->private_switch, nullptr, this, nullptr);
    disconnect(parent, nullptr, this, nullptr);
    set_enabled(false);
}

void JournalReading::start() {
    get_hashtags();
    if (data_ready()) {
        ui->ok->setText("Далее");
        ui->add->setText("Листать");
        ui->skip->setText("Сохранить");
        ui->slider->setMaximum(records.size() - 1);
        ui->page_index->setMaximum(records.size() / ui->pics_per_page->value() + 1);
        display(0);
        load_hashtag_info();
        for (auto record : record_items) {
            ui->view_grid->addWidget(record);
        }
        for (auto title : title_items) {
            ui->title_grid->addWidget(title);
        }
        if (ui->stacked_view->currentIndex() == 1) {
            lay_previews();
        } else if (ui->stacked_view->currentIndex() == 3) {
            lay_titles();
        }
        set_enabled(true);
        show_status();
        if (current_view == START || current_view == PREVIEW) set_view(MAIN);
    } else set_enabled(false);
}

void JournalReading::keyPressEvent(QKeyEvent * event) {
    if (event->key() == Qt::Key_Control) {
        ui->stackedWidget->setCurrentIndex(1);
    }
}

void JournalReading::keyReleaseEvent(QKeyEvent * event) {
    switch (event->key()) {
    case Qt::Key_Control:
        ui->stackedWidget->setCurrentIndex(0);
        break;
    case Qt::Key_PageDown:
        set_view(current_view == MAIN ? LIST : current_view == LIST ? GALLERY : MAIN);
        break;
    case Qt::Key_PageUp:
        set_view(current_view == MAIN ? GALLERY : current_view == LIST ? MAIN : LIST);
        break;
    case Qt::Key_F1:
        emit hashtags["тема_1_лицо"]->hashtagEvent('#',"тема_1_лицо");
        break;
    case Qt::Key_F2:
        emit hashtags["тема_2_лицо"]->hashtagEvent('#',"тема_2_лицо");
        break;
    case Qt::Key_F3:
        emit hashtags["тема_3_лицо"]->hashtagEvent('#',"тема_3_лицо");
        break;
    case Qt::Key_F4:
        emit hashtags["тема_безличная"]->hashtagEvent('#',"тема_безличная");
        break;
    case Qt::Key_F5:
        emit hashtags["тема_объект"]->hashtagEvent('#',"тема_объект");
        break;
    case Qt::Key_Home:
        ui->alphabet_order->trigger();
        break;
    case Qt::Key_End:
        ui->hashtags_full->trigger();
        break;
    default:
        break;
    }
}

void JournalReading::closeEvent(QCloseEvent * event) {
    if (unsaved_changes()) {
        event->ignore();
        if (QMessageBox::question(parent,
                                  "Подтверждение выхода",
                                  "В конфигурационном файле есть несохранённые изменения.\nВыйти без сохранения?",
                                  QMessageBox::Yes | QMessageBox::No) == QMessageBox::Yes) {
            event->accept();
        }
    }
}

void JournalReading::slider_change(int value) {
    pic_end_index = 0;
    pic_index = value;
    display(pic_index);
    update_current_hashtags();
    show_status();
}

void JournalReading::add_button() {
    ++pic_end_index;
    display(pic_index);
    if (!filtration_results.isEmpty() && filtration_results.find(pic_index) == filtration_results.begin()) {
        ui->back->setDisabled(true);
    }
    show_status();
}

void JournalReading::back_button() {
    pic_end_index = 0;
    if (ui->text->isEnabled()) {
        ui->slider->setValue(pic_index - 1);
    } else {
        auto it = --filtration_results.find(pic_index);
        ui->slider->setValue(it.key());
        ui->back->setEnabled(it != filtration_results.begin());
    }
    show_status();
}

void JournalReading::ok_button() {
    if (record_edited) {
        update_record();
        if (pic_index + 1 == records.size()) {
            ui->ok->setEnabled(false);
            ui->save->setEnabled(true);
            return;
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
    show_status();
}

void JournalReading::set_enabled(bool enable) {
    ui->main_view->setEnabled(enable);
    ui->back->setEnabled(enable && pic_index > 0);
    ui->ok->setEnabled(enable);
    ui->add->setEnabled(enable && records[0].pics.size() > 1);
    ui->text->setEnabled(enable);
    ui->private_switch->setEnabled(enable);
    ui->save->setEnabled(enable && !edited_ranges.empty());
    ui->slider->setEnabled(enable);
    ui->slider->setValue(0);
    ui->title_view->setEnabled(enable);
    ui->list_view->setEnabled(enable);
    ui->gallery_view->setEnabled(enable);
    ui->page_index->setEnabled(enable);
    ui->pics_per_page->setEnabled(enable);
    ui->search_bar->setEnabled(enable);
    ui->word_search_button->setEnabled(enable);
    ui->word_search_reset->setEnabled(enable);
    ui->titles_check_all->setEnabled(enable);
    ui->titles_reset_filter->setEnabled(enable);
    ui->titles_set_filter->setEnabled(enable);
    ui->titles_uncheck_all->setEnabled(enable);
    ui->show_public->setEnabled(enable);
    ui->show_private->setEnabled(enable);
    ui->add_caption->setEnabled(enable);
    ui->export_captions_by_ids->setEnabled(enable);
    ui->export_info_by_ids->setEnabled(enable);
    ui->alphabet_order->setEnabled(enable);
    ui->addition_order->setEnabled(enable);
    ui->hashtags_full->setEnabled(enable);
    ui->hashtags_newest->setEnabled(enable);
}

void JournalReading::set_view(View view) {
    if (view == current_view) return;
    current_view = view;
    switch (view) {
    case MAIN:
        ui->stacked_view->setCurrentIndex(1);
        ui->stackedWidget->setCurrentIndex(0);
        break;
    case LIST: case GALLERY:
        ui->stacked_view->setCurrentIndex(2);
        lay_previews(ui->page_index->value());
        break;
    case PREVIEW:
        break;
    case TITLES:
        ui->stacked_view->setCurrentIndex(4);
        lay_titles();
        break;
    default:
        break;
    }
    ui->main_view->setChecked(current_view == MAIN);
    ui->list_view->setChecked(current_view == LIST);
    ui->gallery_view->setChecked(current_view == GALLERY);
    ui->title_view->setChecked(current_view == TITLES);
}

void JournalReading::lay_previews(int page) {
    if (current_view == MAIN) return;
    int pics_per_page = ui->pics_per_page->value();
    int total_previews = filtration_results.isEmpty()
                          ? records.size()
                          : filtration_results.values().size();
    ui->page_index->setMaximum(total_previews / pics_per_page + 1);
    clear_grid(ui->view_grid);
    const auto& items = filtration_results.empty()
                        ? record_items
                        : filtration_results.values();
    for (int i = (page - 1) * pics_per_page ; i < qMin(items.size(), page * pics_per_page); ++i) {
        QtConcurrent::run(items[i], &RecordBase::load_thumbmnail);
        if (current_view == LIST) {
            items[i]->set_list_view();
            ui->view_grid->addWidget(items[i], i, 0);
        } else {
            items[i]->set_gallery_view();
            ui->view_grid->addWidget(items[i], i/10, i%10);
        }
    }
}

void JournalReading::show_status() {
    QString filtration = ui->text->isEnabled()
            ? ""
            : filtration_message(filtration_results.size()) + filtration_indices();
    QString s_rec = QString().setNum(pic_index + 1) + " из " + QString().setNum(records.size());
    QString s_pic = QString().setNum(pic_end_index + 1) + " из " + QString().setNum(records[pic_index].pics.size());
    ui->statusBar->showMessage(filtration + "Запись " + s_rec + ", кадр " + s_pic);
}

void JournalReading::create_hashtag_button(const QString & text) {
    hashtags.insert(text, new HashtagButton(text));
    connect(hashtags[text], SIGNAL(filterEvent(FilterType, const QString&)), this, SLOT(filter_event(FilterType, const QString&)));
    connect(hashtags[text], SIGNAL(hashtagEvent(const QChar&, const QString&)), this, SLOT(hashtag_event(const QChar&, const QString&)));
}

void JournalReading::hashtag_event(const QChar& c, const QString& text) {
    QRegularExpression regex(QString("(.*)?") + c + text + "(\\s.*)?$");
    auto i = regex.globalMatch(ui->text->toPlainText());
    bool hashtag_is_in_text = i.hasNext();
    if (!hashtag_is_in_text) {
        ui->text->setText(ui->text->toPlainText() + " " + c + text);
    } else {
        auto match = i.peekNext();
        ui->text->setText(match.captured(1).chopped(1) + match.captured(2));
    }
    hashtags[text]->highlight(c, !hashtag_is_in_text);
}

bool JournalReading::data_ready() {
    return open_title_journal(record_edited);
}

bool JournalReading::open_title_journal(bool all) {
    QStringList filepaths;
    if (!all) {
        filepaths = QFileDialog::getOpenFileNames(nullptr, "Открыть журнал скриншотов",
                                                           locations[JOURNALS],
                                                           "Файлы (*.json)");
    } else {
        record_edited = false;
        QDir dir = QDir(locations[JOURNALS]);
        dir.setNameFilters(QStringList("*.json"));
        filepaths = dir.entryList(QDir::Files | QDir::NoDotAndDotDot);
        if (filepaths.contains(".test.json")) {
            filepaths.removeAt(filepaths.indexOf(".test.json"));
        }
        for (QString& path : filepaths) {
            path = locations[JOURNALS] + path;
        }
    }
    if (filepaths.isEmpty()) return false;
    for (const auto& filepath : filepaths) {
        auto json_file = json_object(filepath);
        if (!json_file.contains("title") || !json_file.contains("screens")) {
            ui->statusBar->showMessage("Неверный формат файла: " + filepath);
            return false;
        }
        read_title_journal(json_file);
    }
    return !records.empty();
}

void JournalReading::read_title_journal(const QJsonObject& json_file) {
    auto title = json_file.value("title").toString();
    title_map[records.size()] = title;
    auto series = json_file.value("series").toString();
    if (series_map.empty() || series_map.last() != series) {
        series_map[records.size()] = series;
    }
    int album_id = json_file.value("album_id").toInt();
    album_ids[title] = album_id;
    auto title_caption = json_file.value("title_caption").toString();
    title_captions[title] = title_caption;
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
    int title_start_index = records.size() - records_array.size();
    title_items.push_back(new RecordTitleItem(title, path(title_start_index) + records[title_start_index].pics[0], records_array.size(), title_start_index));
    for (int i = title_start_index; i < records.size(); ++i) {
        record_items.push_back(new RecordItem(records[i], i, path(i)));
        connect(record_items[i], &RecordItem::selected, [this](int index){
            ui->slider->setValue(index);
            set_view(MAIN);
        });
    }
}

void JournalReading::save_title_journal(int start, int end) {
    QJsonArray record_array;
    for (int i = start; i <= end; ++i) {
        record_array.push_back(records[i].to_json());
    }
    auto title = title_map.value(start);
    QFile file(locations[JOURNALS] + title + ".json");
    QJsonObject object;
    object["title"] = title;
    object["title_caption"] = title_captions[title];
    object["series"] = series_name(start);
    object["album_id"] = album_ids[title];
    object["screens"] = record_array;
    auto message = save_json(object, file)
            ? "Журнал скриншотов сохранён."
            : QString("Не удалось сохранить файл: %1").arg(file.fileName());
    ui->statusBar->showMessage(message);
}

void JournalReading::display(int index) {
    if (!ui->offline->isChecked()) {
        manager->get_image(records[index].links[pic_end_index]);
    } else {
        auto image = QImage(path(index) + records[index].pics[pic_end_index]);
        if (image.isNull()) image = QImage(QString(path(index) + records[index].pics[pic_end_index]).chopped(3) + "jpg");
        ui->image->setPixmap(scaled(image));
    }
    disconnect(ui->text, &QTextEdit::textChanged, this, &JournalReading::set_edited);
    ui->text->setText(records[index].quote);
    connect(ui->text, &QTextEdit::textChanged, this, &JournalReading::set_edited);
    bool reached_end = index + 1 >= records.size();
    bool listing_on = pic_end_index + 1 < records[index].pics.size();
    ui->save->setEnabled(!edited_ranges.empty());
    disconnect(ui->private_switch, &QAction::triggered, this, &JournalReading::set_edited);
    ui->private_switch->setChecked(!records[index].is_public);
    connect(ui->private_switch, &QAction::triggered, this, &JournalReading::set_edited);
    ui->add->setEnabled(listing_on);
    ui->ok->setEnabled(!reached_end);
    ui->back->setEnabled(index > 0);
}

void JournalReading::set_edited() {
    record_edited = true;
    edited_ranges.insert(title_range(pic_index));
    ui->ok->setEnabled(true);
}

bool JournalReading::update_quote_file(int start, int end) {
    QFile file(locations[QUOTES] + title_map.value(start) + ".txt");
    if (!file.open(QIODevice::WriteOnly | QIODevice::Truncate)) {
        ui->statusBar->showMessage(QString("Не удалось открыть файл с цитатами: %1").arg(file.fileName()));
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

void JournalReading::update_record() {
    auto text = ui->text->toPlainText();
    records[pic_index].quote = text;
    records[pic_index].is_public = !ui->private_switch->isChecked();
    record_items[pic_index]->update_text(text);
    record_edited = false;
    update_hashtag_info();
}

void JournalReading::save_changes() {
    qDebug() << edited_ranges << records.size();
    for (const auto& pair : edited_ranges) {
        qDebug() << pair;
        int start = pair.first;
        int end = pair.second;
        update_quote_file(start, end);
        save_title_journal(start, end);
        record_edited = false;
        ui->save->setEnabled(false);
    }
    edited_ranges.clear();
}

void JournalReading::show_public(bool checked) {
    if (checked && ui->show_private->isChecked()) {
        ui->show_private->setChecked(false);
    }
    filter_event(true);
}

void JournalReading::show_private(bool checked) {
    if (checked && ui->show_public->isChecked()) {
        ui->show_public->setChecked(false);
    }
    filter_event(false);
}

void JournalReading::add_caption(const QString& captcha_sid, const QString& captcha_key) {
    manager->edit_photo_caption(captions_for_ids.firstKey(), captions_for_ids.first(), captcha_sid, captcha_key);
}

void JournalReading::caption_success() {
    captions_for_ids.remove(captions_for_ids.firstKey());
    if (!captions_for_ids.empty()) {
        QThread::msleep(350);
        add_caption();
    } else {
        ui->statusBar->showMessage("Добавление подписей прошло успешно.");
    }
}

void JournalReading::captcha_handling(const QString& captcha_id) {
    ui->statusBar->showMessage(QString("Осталось подписать %1 фотографий").arg(captions_for_ids.size()));
    bool ok;
    QString text = QInputDialog::getText(parent, tr("Капча"),
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

QString JournalReading::preprocessed(const QString& text) const {
    QString result = text;
    {
        // Managing textless records
        if ((result[0] == '#' || result.startsWith(" #") || result.startsWith(" &")) && !result.contains("#без_текста")) {
            result += " #без_текста";
            qDebug() << result;
        }
    }
    {
        // Setting #вопрос
        QRegularExpression question_regex("(.*)?(\\?!?\\s#)(.*)?$");
        auto i = question_regex.globalMatch(result);
        if (i.hasNext()) {
            auto match = i.peekNext();
            if (!match.captured(3).contains("вопрос")) {
                result = match.captured(1) + "?" + (match.captured(2).at(1) == '!' ? "!" : "") + " #вопрос #" + match.captured(3);
                qDebug() << result;
            }
        }
    }
    return result;
}

void JournalReading::load_hashtag_info() {
    AbstractOperationMode::load_hashtag_info();
    highlight_current_hashtags(false);
    current_hashtags = hashtags_by_index[0];
    highlight_current_hashtags(true);
}

void JournalReading::update_hashtag_info() {
    recalculate_hashtags(false);
    update_current_hashtags();
    recalculate_hashtags(true);
}

void JournalReading::update_current_hashtags() {
    highlight_current_hashtags(false);
    current_hashtags.clear();
    auto i = hashtag_match(records[pic_index].quote);
    while (i.hasNext()) {
        current_hashtags.push_back(i.next().captured());
    }
    hashtags_by_index[pic_index] = current_hashtags;
    highlight_current_hashtags(true);
}

void JournalReading::highlight_current_hashtags(bool enable) {
    for (auto tag : current_hashtags) {
        auto hashtag = tag.right(tag.size()-1);
        if (!hashtags.contains(hashtag)) {
            qDebug() << "Unexpected tag: " << hashtag;
            create_hashtag_button(hashtag);
            hashtags[hashtag]->highlight_unregistered();
            ranked_hashtags.last().append(hashtag);
            update_hashtag_grid();
        }
        auto button = hashtags.value(hashtag);
        button->highlight(tag.front(), enable);
    }
}

void JournalReading::recalculate_hashtags(bool increase) {
    for (const auto& tag : current_hashtags) {
        auto hashtag = tag.right(tag.size()-1);
        if (increase) {
            hashtags[hashtag]->add_index(tag.front(), pic_index);
        } else {
            hashtags[hashtag]->remove_index(tag.front(), pic_index);
        }
        auto button = hashtags.value(hashtag);
        if (button) {
            button->show_count();
        }
    }
}

void JournalReading::export_captions_by_ids() {
    QJsonObject captions;
    QJsonObject links;
    for (int i = 0; i < records.size(); ++i) {
        for (int j = 0; j < records[i].ids.size(); ++j) {
            QString id = QString().setNum(records[i].ids[j]);
            QString caption;
            QRegularExpression regex("(.*?)?([#&])(.*)?$");
            auto it = regex.globalMatch(records[i].quote);
            if (it.hasNext()) {
                auto match = it.peekNext();
                caption = match.captured(1);
            } else caption = records[i].quote;
            captions[id] = caption;
            links[id] = records[i].links[j];
        }
    }
    QFile file_captions(locations[JOURNALS] + "result\\captions_list.json");
    QFile file_links(locations[JOURNALS] + "result\\links_list.json");
    auto message = save_json(captions, file_captions) && save_json(links, file_links)
            ? "Список текстов по кадрам сохранён."
            : "Не удалось сохранить файл.";
    ui->statusBar->showMessage(message);
}

void JournalReading::export_info_by_ids() {
    QJsonObject captions;
    for (int i = 0; i < records.size(); ++i) {
        for (int j = 0; j < records[i].ids.size(); ++j) {
            QString id = QString().setNum(records[i].ids[j]);
            QString caption;
            QJsonObject info;
            QRegularExpression regex("(.*?)? ([#&])(.*)?$");
            auto it = regex.globalMatch(records[i].quote);
            if (it.hasNext()) {
                auto match = it.peekNext();
                caption = match.captured(1);
            } else caption = records[i].quote;
            info["caption"] = caption;
            info["link"] = records[i].links[j];
            info["path"] = title_name(i) + QDir::separator() + records[i].pics[j];
            info["id_prev"] = j > 0 ? records[i].ids[j-1] : 0;
            info["id_next"] = j + 1 < records[i].ids.size() ? records[i].ids[j+1] : 0;
            captions[id] = info;
        }
    }
    QFile file(locations[JOURNALS] + "result\\photo_info.json");
    auto message = save_json(captions, file)
            ? "Информация по кадрам сохранена."
            : "Не удалось сохранить файл.";
    ui->statusBar->showMessage(message);
}
