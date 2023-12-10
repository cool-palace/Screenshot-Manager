#include "mainwindow.h"
#include "ui_mainwindow.h"
#include <QDebug>

MainWindow::MainWindow(QWidget *parent)
    : QMainWindow(parent)
    , ui(new Ui::MainWindow)
{
    ui->setupUi(this);
    if (initialize()) {
        get_hashtags();
    }
    connect(manager, &VK_Manager::albums_ready, [this](const QMap<QString, int>& ids) {
        album_ids = ids;
        qDebug() << ids;
        if (album_ids.empty()) {
            ui->offline->setChecked(true);
            ui->statusBar->showMessage("Не удалось загрузить альбомы. Попробуйте авторизироваться вручную или продолжите работу оффлайн.");
        }
    });

    connect(manager, &VK_Manager::photo_ids_ready, [this](const QVector<int>& ids, const QStringList& urls) {
        photo_ids = ids;
        links = urls;
        set_mode(data_ready() ? CONFIG_CREATION : IDLE);
    });

    connect(manager, &VK_Manager::image_ready, [this](const QImage& image) {
        ui->image->setPixmap(scaled(image));
    });

    connect(manager, &VK_Manager::posted_successfully, [this](int index, int date) {
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
    });

    connect(manager, &VK_Manager::poll_posted_successfully, [this]() {
        ui->statusBar->showMessage(QString("Опрос опубликован"));
    });

    connect(manager, &VK_Manager::post_failed, [this](int index, const QString& reply) {
        if (post_counter < selected_records.size()) {
            update_logs();
            post_counter = 0;
        }
        ui->statusBar->showMessage(QString("Не удалось опубликовать запись %1").arg(index+1));
        QMessageBox msgBox(QMessageBox::Critical, "Ошибка", reply);
        msgBox.exec();
    });

    connect(manager, &VK_Manager::poll_post_failed, [this](const QString& reply) {
        ui->statusBar->showMessage(QString("Не удалось опубликовать опрос"));
        QMessageBox msgBox(QMessageBox::Critical, "Ошибка", reply);
        msgBox.exec();
    });

    connect(manager, &VK_Manager::poll_ready, [this](int id) {
        int time = QDateTime(ui->date->date(), ui->time->time(), Qt::LocalTime).toSecsSinceEpoch();
        manager->post(poll_message(), id, time);
    });

    connect(ui->config_creation, &QAction::triggered, [this]() {
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
    });

    connect(ui->config_reading, &QAction::triggered, [this]() {
        if (!open_title_config()) {
            set_mode(IDLE);
            ui->statusBar->showMessage("Конфигурационный файл не открыт.");
            return;
        }
        set_mode(CONFIG_READING);
        if (current_view == PREVIEW) set_view(MAIN);
    });

    connect(ui->config_reading_all, &QAction::triggered, [this]() {
        if (!open_title_config(true)) {
            set_mode(IDLE);
            ui->statusBar->showMessage("Конфигурационный файл не открыт.");
            return;
        }
        set_mode(CONFIG_READING);
        if (current_view == PREVIEW) set_view(MAIN);
    });

    connect(ui->text_reading, &QAction::triggered, [this]() {
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
    });

    connect(ui->release_preparation, &QAction::triggered, [this]() {
        clear_all();
        if (!open_public_config()) {
            set_mode(IDLE);
            ui->statusBar->showMessage("Конфигурационный файл не открыт.");
            return;
        }
        set_mode(RELEASE_PREPARATION);
        set_view(PREVIEW);
    });

    connect(ui->poll_preparation, &QAction::triggered, [this]() {
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
    });

    connect(ui->save, &QAction::triggered, this, &MainWindow::save_changes);
    connect(ui->compile, &QAction::triggered, this, &MainWindow::compile_configs);
    connect(ui->export_text, &QAction::triggered, this, &MainWindow::export_text);
    connect(ui->add_hashtag, &QAction::triggered, [this]() {
        bool ok;
        QString text = QInputDialog::getText(this, tr("Добавление хэштега"),
                                                   tr("Введите новый хэштег:"), QLineEdit::Normal,
                                                   "", &ok);
        if (ok && !text.isEmpty() && !hashtags.contains(text)) {
            create_hashtag_button(text);
            update_hashtag_grid();
        }
    });

    connect(ui->add_caption, &QAction::triggered, [this]() {
        add_caption();
    });

    connect(manager, &VK_Manager::caption_passed, [this]() {
        captions_for_ids.remove(captions_for_ids.firstKey());
        if (!captions_for_ids.empty()) {
            QThread::msleep(350);
            add_caption();
        } else {
            ui->statusBar->showMessage("Добавление подписей прошло успешно.");
        }
    });

    connect(manager, &VK_Manager::captcha_error, [this](const QString& captcha_id) {
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
    });

    connect(ui->show_public, &QAction::triggered, [this](bool checked) {
        if (checked && ui->show_private->isChecked()) {
            ui->show_private->setChecked(false);
        }
        filter_event(true);
    });
    connect(ui->show_private, &QAction::triggered, [this](bool checked) {
        if (checked && ui->show_public->isChecked()) {
            ui->show_public->setChecked(false);
        }
        filter_event(false);
    });

    connect(ui->slider, &QAbstractSlider::valueChanged, [this](int value) {
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
    });

    connect(ui->page_index, SIGNAL(valueChanged(int)), this, SLOT(lay_previews(int)));
    connect(ui->search_bar, &QLineEdit::editingFinished, [this]() {
        filter_event(ui->search_bar->text());
        lay_previews();
    });
//    connect(ui->refactor, &QAction::triggered, this, &MainWindow::refactor_configs);

    connect(ui->main_view, &QAction::triggered, [this]() { set_view(MAIN); });
    connect(ui->list_view, &QAction::triggered, [this]() { set_view(LIST); });
    connect(ui->gallery_view, &QAction::triggered, [this]() { set_view(GALLERY); });
    connect(ui->preview_view, &QAction::triggered, [this]() { set_view(PREVIEW); });

    connect(ui->alphabet_order, &QAction::triggered, [this]() {
        ui->addition_order->setChecked(!ui->alphabet_order->isChecked());
        update_hashtag_grid();
    });
    connect(ui->addition_order, &QAction::triggered, [this]() {
        ui->alphabet_order->setChecked(!ui->addition_order->isChecked());
        update_hashtag_grid();
    });

    connect(ui->hashtags_full, &QAction::triggered, [this]() {
        ui->hashtags_newest->setChecked(!ui->hashtags_full->isChecked());
        update_hashtag_grid();
    });
    connect(ui->hashtags_newest, &QAction::triggered, [this]() {
        ui->hashtags_full->setChecked(!ui->hashtags_newest->isChecked());
        update_hashtag_grid();
    });

    connect(ui->private_switch, &QAction::triggered, [this] () {
        if (current_mode == CONFIG_READING)
            set_edited();
    });

    connect(ui->load_subs, &QAction::triggered, [this] () {
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
    });

    connect(ui->generate, &QPushButton::clicked, [this]() {
        QLayoutItem* child;
        while ((child = ui->preview_grid->takeAt(0))) {
            // Clearing items from the grid
            child->widget()->hide();
        }
        QDateTime time = QDateTime(ui->date->date(), ui->time->time(), Qt::LocalTime);
        if (!ui->poll_preparation->isChecked()) {
            for (auto record : selected_records) {
                delete record;
            }
            selected_records.clear();
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
        } else {
            for (auto tag : selected_hashtags) {
                delete tag;
            }
            selected_hashtags.clear();
            for (int i = 0; i < ui->quantity->value(); ++i) {
                int r_index = QRandomGenerator::global()->bounded(full_hashtags.size());
                auto tag = full_hashtags[r_index];
                selected_hashtags[tag.tag()] = new HashtagPreview(tag);
            }
            for (const auto& tag : selected_hashtags) {
                ui->preview_grid->addWidget(tag);
            }
        }
    });

    connect(ui->post, &QPushButton::clicked, [this]() {
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
    });

    connect(ui->skip, &QPushButton::clicked, [this]() {
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
    });

    connect(ui->add, &QPushButton::clicked, [this]() {
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
    });

    connect(ui->back, &QPushButton::clicked, [this]() {
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
    });

    connect(ui->ok, &QPushButton::clicked, [this]() {
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
    });
}

MainWindow::~MainWindow() {
    delete ui;
    delete manager;
}

void MainWindow::keyPressEvent(QKeyEvent* event) {
    if (event->key() == Qt::Key_Control) {
        if (current_mode == CONFIG_READING) {
            ui->stackedWidget->setCurrentIndex(1);
        } else if (current_mode == CONFIG_CREATION) {
            ui->back->setText("Отмена");
        } else if (current_mode == RELEASE_PREPARATION && current_view != PREVIEW) {
            set_view(MAIN);
        }
    }
}

void MainWindow::keyReleaseEvent(QKeyEvent* event) {
    switch (event->key()) {
    case Qt::Key_Control:
        if (current_mode == CONFIG_READING) {
            ui->stackedWidget->setCurrentIndex(0);
        } else if (current_mode == CONFIG_CREATION) {
            ui->back->setText("Назад");
        } else if (current_mode == RELEASE_PREPARATION && current_view != PREVIEW) {
            set_view(LIST);
        }
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
    case Qt::Key_Enter:
        emit ui->ok->clicked();
        break;
    case Qt::Key_Left:
        emit ui->back->clicked();
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

void MainWindow::closeEvent(QCloseEvent *event) {
    if (!edited_ranges.empty()) {
        event->ignore();
        if (QMessageBox::question(this,
                                  "Подтверждение выхода",
                                  "В конфигурационном файле есть несохранённые изменения.\nВыйти без сохранения?",
                                  QMessageBox::Yes | QMessageBox::No) == QMessageBox::Yes) {
            event->accept();
        }
    }
}

bool MainWindow::initialize() {
    auto json_file = json_object("config.json");
    if (!json_file.contains("screenshots") /*|| !json_file.contains("docs")*/ || !json_file.contains("configs")) {
        ui->statusBar->showMessage("Неверный формат конфигурационного файла.");
        return false;
    }
    locations[SCREENSHOTS] = json_file.value("screenshots").toString();
    locations[QUOTES] = json_file.value("docs").toString();
    locations[SUBS] = json_file.value("subs").toString();
    locations[CONFIGS] = json_file.value("configs").toString();
    locations[LOGS] = json_file.value("logs").toString();
    QString access_token = json_file.value("access_token").toString();
    QString group_id = json_file.value("group_id").toString();
    QString public_id = json_file.value("public_id").toString();
    manager = new VK_Manager(access_token, group_id, public_id);
    RecordFrame::manager = manager;
    manager->get_albums();
    load_special_titles();
    if (!QDir(locations[SCREENSHOTS]).exists() || !QDir(locations[CONFIGS]).exists()) {
        ui->statusBar->showMessage("Указаны несуществующие директории. Перепроверьте конфигурационный файл.");
        return false;
    }
    ui->statusBar->showMessage("Конфигурация успешно загружена.");
    return true;
}

void MainWindow::clear_all() {
    quotes.clear();
    photo_ids.clear();
    pics.clear();
    links.clear();
    records.clear();
    title_map.clear();
    subs.clear();
    hashtags_by_index.clear();
    filters.clear();
    filtration_results.clear();
    while (ui->view_grid->takeAt(0) != nullptr) {
        // Clearing items from the grid
    }
    for (auto item : record_items) {
        delete item;
    }
    record_items.clear();
}

void MainWindow::set_mode(Mode mode) {
    current_mode = mode;
    quote_index = pic_index = pic_end_index = 0;
    switch (mode) {
    case CONFIG_CREATION:
        ui->ok->setText("Готово");
        ui->add->setText("Добавить");
        ui->skip->setText("Пропустить");
        ui->slider->setMaximum(pics.size() - 1);
        draw(0);
        break;
    case CONFIG_READING:
        ui->ok->setText("Далее");
        ui->add->setText("Листать");
        ui->skip->setText("Сохранить");
        ui->slider->setMaximum(records.size() - 1);
        ui->page_index->setMaximum(records.size() / pics_per_page + 1);
        display(0);
        load_hashtag_info();
        for (auto record : record_items) {
            ui->view_grid->addWidget(record);
        }
        if (ui->stacked_view->currentIndex() > 0) {
            lay_previews();
        }
        break;
    case TEXT_READING:
        ui->ok->setText("Готово");
        ui->skip->setText("Предыдущий");
        ui->add->setText("Следующий");
//        ui->slider->setMaximum(pics.size() - 1);
        draw(0);
        show_text(0);
//        for (auto record : record_items) {
//            ui->view_grid->addWidget(record);
//        }
        break;
    case RELEASE_PREPARATION:
        load_hashtag_info();
        ui->date->setMinimumDate(QDate::currentDate());
        ui->date->setDate(QTime::currentTime() < QTime(3,0)
                              ? QDate::currentDate()
                              : QDate::currentDate().addDays(1));
    {
        bool weekend = ui->date->date().dayOfWeek() > 5;
        ui->time->setTime(weekend ? QTime(10,0) : QTime(8,0));
        if (weekend) ui->quantity->setValue(6);
    }
        RecordPreview::records = &records;
        ui->generate->click();
        break;
    default:
        break;
    }
    set_enabled(mode);
    show_status();
}

void MainWindow::set_view(View view) {
    if (view == current_view) return;
    current_view = view;
    switch (view) {
    case MAIN:
        ui->stacked_view->setCurrentIndex(0);
        if (current_mode == RELEASE_PREPARATION) {
            ui->stackedWidget->setCurrentIndex(1);
        }
        break;
    case LIST: case GALLERY:
        ui->stacked_view->setCurrentIndex(1);
        lay_previews();
        break;
    case PREVIEW:
        if (current_mode == RELEASE_PREPARATION) {
            ui->stacked_view->setCurrentIndex(2);
        }
        break;
    }
    ui->main_view->setChecked(current_view == MAIN);
    ui->list_view->setChecked(current_view == LIST);
    ui->gallery_view->setChecked(current_view == GALLERY);
    ui->preview_view->setChecked(current_view == PREVIEW);
}

void MainWindow::lay_previews(int page) {
    if (current_view == MAIN || current_mode == IDLE) return;
    int total_previews = current_mode == CONFIG_READING
                            ? (filtration_results.isEmpty()
                                ? records.size()
                                : filtration_results.values().size())
                            : record_items.size();
    ui->page_index->setMaximum(total_previews / pics_per_page + 1);
    QLayoutItem* child;
    while ((child = ui->view_grid->takeAt(0))) {
        // Clearing items from the grid
        child->widget()->hide();
    }
    const auto& items = filtration_results.empty()
                        ? record_items
                        : filtration_results.values();
    for (int i = (page - 1) * pics_per_page ; i < qMin(items.size(), page * pics_per_page); ++i) {
        if (current_view == LIST) {
            items[i]->set_list_view();
            ui->view_grid->addWidget(items[i], i, 0);
        } else {
            items[i]->set_gallery_view();
            ui->view_grid->addWidget(items[i], i/10, i%10);
        }
    }
}

void MainWindow::set_enabled(bool enable) {
    ui->back->setEnabled(enable && pic_index > 0 && current_mode != RELEASE_PREPARATION);
    ui->ok->setEnabled(enable && current_mode != RELEASE_PREPARATION);
    ui->skip->setEnabled(enable && current_mode == CONFIG_CREATION);
    bool listing_enabled = (current_mode == CONFIG_CREATION && pics.size() > 1)
            || (current_mode == CONFIG_READING && records[0].pics.size() > 1);
    ui->add->setEnabled(enable && listing_enabled);
    ui->text->setEnabled(enable && current_mode != TEXT_READING && current_mode != RELEASE_PREPARATION);
    ui->private_switch->setEnabled(current_mode == CONFIG_CREATION || current_mode == CONFIG_READING);
    ui->load_subs->setEnabled(current_mode == TEXT_READING);
    ui->slider->setEnabled(current_mode == CONFIG_READING);
    ui->slider->setValue(0);
    ui->preview_view->setEnabled(current_mode == RELEASE_PREPARATION);
    ui->poll_preparation->setEnabled(current_mode == RELEASE_PREPARATION);
}

void MainWindow::set_edited() {
    record_edited = true;
    edited_ranges.insert(title_range(pic_index));
    ui->ok->setEnabled(true);
}

QString MainWindow::title_name(int index) {
    if (title_map.contains(index)) return title_map.value(index);
    title_map[index] = QString();
    auto it = title_map.find(index);
    auto title = (--it).value();
    title_map.remove(index);
    return title;
}

QPair<int, int> MainWindow::title_range(int index) {
    // Returns starting and ending indices for a title containing records[index]
    int start, end;
    if (title_map.contains(index)) {
        auto it = title_map.find(index);
        if (++it != title_map.end()) {
            end = it.key() - 1;
        } else end = records.size() - 1;
        return qMakePair(index, end);
    }
    title_map[index] = QString();
    auto it = title_map.find(index);
    start = (--it).key();
    title_map.remove(index);
    if (++it != title_map.end()) {
        end = it.key() - 1;
    } else end = records.size() - 1;
    return qMakePair(start, end);
}

void MainWindow::save_changes() {
    qDebug() << edited_ranges << records.size();
    for (const auto& pair : edited_ranges) {
        qDebug() << pair;
        int start = pair.first;
        int end = pair.second;
        update_quote_file(start, end);
        save_title_config(start, end);
        record_edited = false;
        ui->save->setEnabled(false);
    }
    edited_ranges.clear();
}

int MainWindow::random_index() const {
    if (filtration_results.empty()) {
        // Getting random index from records
        return QRandomGenerator::global()->bounded(records.size());
    }
    // Getting random index from filtered records only
    auto filtered_indices = filtration_results.keys();
    int random_pre_index = QRandomGenerator::global()->bounded(filtered_indices.size());
    return filtered_indices[random_pre_index];
}

QString MainWindow::path(int index) {
    return locations[SCREENSHOTS] + title_name(index) + QDir::separator();
}

QPixmap MainWindow::scaled(const QImage& source) const {
    return QPixmap::fromImage(source.scaled(ui->image->geometry().size(), Qt::KeepAspectRatio));
}

QJsonObject MainWindow::json_object(const QString& filepath) const {
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

bool MainWindow::save_json(const QJsonObject& object, QFile& file) const {
    if (!file.open(QIODevice::WriteOnly | QIODevice::Truncate)) {
        return false;
    }
    QTextStream out(&file);
    out.setCodec("UTF-8");
    out << QJsonDocument(object).toJson();
    file.close();
    return true;
}

void MainWindow::load_special_titles() {
    auto json_file = json_object(locations[CONFIGS] + "\\result\\titles.json");
    for (auto key : json_file.keys()) {
        special_titles[key] = json_file.value(key).toString();
    }
}

void MainWindow::add_caption(const QString& captcha_sid, const QString& captcha_key) {
    manager->edit_photo_caption(captions_for_ids.firstKey(), captions_for_ids.first(), captcha_sid, captcha_key);
}

bool MainWindow::data_ready() {
    if (pics.empty()) {
        ui->statusBar->showMessage("Выбранная папка не содержит кадров.");
        return false;
    }
    if (quotes.empty()) {
        ui->statusBar->showMessage("Не удалось загрузить документ с цитатами.");
        return false;
    }
    if (!ui->offline->isChecked() && pics.size() != photo_ids.size() && !photo_ids.empty()) {
        ui->statusBar->showMessage("Необходимо провести синхронизацию локального и облачного хранилища.");
        return false;
    }
    if (pics.size() < quotes.size()) {
        ui->statusBar->showMessage("Необходимо проверить состав кадров и цитат.");
        return false;
    }
    if (photo_ids.empty()) {
        if (!ui->offline->isChecked()) {
            ui->statusBar->showMessage("Не удалось получить идентификаторы кадров. ");
            return false;
        } else {
            photo_ids.resize(pics.size());
        }
    }
    return true;
}

QString MainWindow::filtration_message(int i) const {
    QString found = "Найдено ";
    QString recs = " записей по ";
    QString filters_count = " фильтрам";
    if ((i % 100 - i % 10) != 10) {
        if (i % 10 == 1) {
            found = "Найдена ";
            recs = " запись по ";
        } else if (i % 10 > 1 && i % 10 < 5) {
            recs = " записи по ";
        }
    }
    int j = filters.size();
    if ((j % 100 - j % 10) != 10 && j % 10 == 1) {
        filters_count = " фильтру";
    }
    return found + QString().setNum(i) + recs + QString().setNum(j) + filters_count;
}

QString MainWindow::filtration_indices() const {
    if (filtration_results.isEmpty()) return ". ";
    QString result = ": (";
    int buffer = -2;
    bool range_active = false;
    for (int index : filtration_results.keys()) {
        if (index == buffer + 1) {
            range_active = true;
            if (index == filtration_results.keys().last()) {
                result.append('-' + QString().setNum(index + 1));
            } else buffer = index;
        } else if (range_active) {
            result.append('-' + QString().setNum(buffer + 1));
            result.append(", " + QString().setNum(index + 1));
            range_active = false;
        } else {
            if (result.size() > 3) result.append(", ");
            result.append(QString().setNum(index + 1));
        }
        buffer = index;
    }
    result.append("). ");
    return result;
}

void MainWindow::show_status() {
    switch (current_mode) {
    case CONFIG_CREATION: {
        bool multiple_pics = pic_end_index > 0;
        QString s_quote = QString().setNum(quote_index + 1) + " из " + QString().setNum(quotes.size());
        QString s_pic = multiple_pics ? "кадры " : "кадр ";
        QString s_pic_index = QString().setNum(pic_index + 1) + (multiple_pics ? "-" + QString().setNum(pic_end_index + 1) : "");
        QString s_pic_from = " из " + QString().setNum(pics.size());
        ui->statusBar->showMessage("Цитата " + s_quote + ", " + s_pic + s_pic_index + s_pic_from);
        break;
    }
    case CONFIG_READING: {
        QString filtration = ui->text->isEnabled()
                ? ""
                : filtration_message(filtration_results.size()) + filtration_indices();
        QString s_rec = QString().setNum(pic_index + 1) + " из " + QString().setNum(records.size());
        QString s_pic = QString().setNum(pic_end_index + 1) + " из " + QString().setNum(records[pic_index].pics.size());
        ui->statusBar->showMessage(filtration + "Запись " + s_rec + ", кадр " + s_pic);
        break;
    }
    case TEXT_READING: {
        QString s_line = subs.isEmpty()
                ? ""
                : ", строка " + QString().setNum(quote_index + 1) + " из " + QString().setNum(subs.size());;
        QString s_pic = QString().setNum(pic_index + 1) + " из " + QString().setNum(pics.size());
        ui->statusBar->showMessage("Кадр " + s_pic + s_line);
        break;
    }
    default:
        break;
    }
}
