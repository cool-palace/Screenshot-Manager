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
    connect(manager, &VK_Manager::albums_ready, this, &MainWindow::set_albums);
    connect(manager, &VK_Manager::photo_ids_ready, this, &MainWindow::set_photo_ids);
    connect(manager, &VK_Manager::image_ready, this, &MainWindow::set_loaded_image);
    connect(manager, &VK_Manager::posted_successfully, this, &MainWindow::posting_success);
    connect(manager, &VK_Manager::post_failed, this, &MainWindow::posting_fail);
    connect(manager, &VK_Manager::poll_ready, this, &MainWindow::post_poll);
    connect(manager, &VK_Manager::poll_posted_successfully, this, &MainWindow::poll_posting_success);
    connect(manager, &VK_Manager::poll_post_failed, this, &MainWindow::poll_posting_fail);
    connect(manager, &VK_Manager::caption_passed, this, &MainWindow::caption_success);
    connect(manager, &VK_Manager::captcha_error, this, &MainWindow::captcha_handling);

    connect(ui->config_creation, &QAction::triggered, this, &MainWindow::journal_creation);
    connect(ui->config_reading, &QAction::triggered, this, &MainWindow::journal_reading);
    connect(ui->config_reading_all, &QAction::triggered, this, &MainWindow::journal_reading_all);
    connect(ui->text_reading, &QAction::triggered, this, &MainWindow::text_reading);
    connect(ui->release_preparation, &QAction::triggered, this, &MainWindow::release_preparation);
    connect(ui->poll_preparation, &QAction::triggered, this, &MainWindow::poll_preparation);

    connect(ui->save, &QAction::triggered, this, &MainWindow::save_changes);
    connect(ui->compile, &QAction::triggered, this, &MainWindow::compile_journals);
    connect(ui->export_text, &QAction::triggered, this, &MainWindow::export_text);
    connect(ui->add_hashtag, &QAction::triggered, this, &MainWindow::add_hashtag);

    connect(ui->add_caption, &QAction::triggered, [this]() {
        add_caption();
    });

    connect(ui->show_public, &QAction::triggered, this, &MainWindow::show_public);
    connect(ui->show_private, &QAction::triggered, this, &MainWindow::show_private);
    connect(ui->slider, &QAbstractSlider::valueChanged, this, &MainWindow::slider_change);

    connect(ui->page_index, QOverload<int>::of(&QSpinBox::valueChanged), this, &MainWindow::lay_previews);
    connect(ui->search_bar, &QLineEdit::editingFinished, [this]() {
        filter_event(ui->search_bar->text());
        lay_previews();
    });
//    connect(ui->refactor, &QAction::triggered, this, &MainWindow::refactor_configs);

    connect(ui->main_view, &QAction::triggered, [this]() { set_view(MAIN); });
    connect(ui->list_view, &QAction::triggered, [this]() { set_view(LIST); });
    connect(ui->gallery_view, &QAction::triggered, [this]() { set_view(GALLERY); });
    connect(ui->preview_view, &QAction::triggered, [this]() { set_view(PREVIEW); });
    connect(ui->title_view, &QAction::triggered, [this]() { set_view(TITLES); });

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
        if (current_mode == JOURNAL_READING)
            set_edited();
    });

    connect(ui->titles_check_all, &QPushButton::clicked, [this]() { check_titles(true); });
    connect(ui->titles_uncheck_all, &QPushButton::clicked, [this]() { check_titles(false); });
    connect(ui->titles_set_filter, &QPushButton::clicked, [this]() { filter_event(nullptr); }); // Check type

    connect(ui->load_subs, &QAction::triggered, this, &MainWindow::load_subs);
    connect(ui->generate, &QPushButton::clicked, this, &MainWindow::generate_button);
    connect(ui->post, &QPushButton::clicked, this, &MainWindow::post_button);
    connect(ui->skip, &QPushButton::clicked, this, &MainWindow::skip_button);
    connect(ui->add, &QPushButton::clicked, this, &MainWindow::add_button);
    connect(ui->back, &QPushButton::clicked, this, &MainWindow::back_button);
    connect(ui->ok, &QPushButton::clicked, this, &MainWindow::ok_button);
}

MainWindow::~MainWindow() {
    delete ui;
    delete manager;
}

void MainWindow::keyPressEvent(QKeyEvent* event) {
    if (event->key() == Qt::Key_Control) {
        if (current_mode == JOURNAL_READING) {
            ui->stackedWidget->setCurrentIndex(1);
        } else if (current_mode == JOURNAL_CREATION) {
            ui->back->setText("Отмена");
        } else if (current_mode == RELEASE_PREPARATION && current_view != PREVIEW) {
            set_view(MAIN);
        }
    }
}

void MainWindow::keyReleaseEvent(QKeyEvent* event) {
    switch (event->key()) {
    case Qt::Key_Control:
        if (current_mode == JOURNAL_READING) {
            ui->stackedWidget->setCurrentIndex(0);
        } else if (current_mode == JOURNAL_CREATION) {
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
    locations[JOURNALS] = json_file.value("configs").toString();
    locations[LOGS] = json_file.value("logs").toString();
    locations[POLL_LOGS] = json_file.value("poll_logs").toString();
    QString access_token = json_file.value("access_token").toString();
    QString group_id = json_file.value("group_id").toString();
    QString public_id = json_file.value("public_id").toString();
    manager = new VK_Manager(access_token, group_id, public_id);
    RecordFrame::manager = manager;
    manager->get_albums();
    load_special_titles();
    if (!QDir(locations[SCREENSHOTS]).exists() || !QDir(locations[JOURNALS]).exists()) {
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
    clear_grid(ui->view_grid);
    for (auto item : record_items) {
        delete item;
    }
    record_items.clear();
    clear_grid(ui->title_grid);
    for (auto item : title_items) {
        delete item;
    }
    title_items.clear();
}

void MainWindow::clear_grid(QLayout* layout, bool hide) {
    QLayoutItem* child;
    while ((child = layout->takeAt(0))) {
        // Clearing items from the grid
        if (hide) child->widget()->hide();
    }
}

void MainWindow::set_mode(Mode mode) {
    current_mode = mode;
    quote_index = pic_index = pic_end_index = 0;
    switch (mode) {
    case JOURNAL_CREATION:
        ui->ok->setText("Готово");
        ui->add->setText("Добавить");
        ui->skip->setText("Пропустить");
        ui->slider->setMaximum(pics.size() - 1);
        ui->stackedWidget->setCurrentIndex(0);
        draw(0);
        break;
    case JOURNAL_READING:
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
        for (auto title : title_items) {
            ui->title_grid->addWidget(title);
        }
        if (ui->stacked_view->currentIndex() == 1) {
            lay_previews();
        } else if (ui->stacked_view->currentIndex() == 3) {
            lay_titles();
        }
        break;
    case TEXT_READING:
        ui->ok->setText("Готово");
        ui->skip->setText("Предыдущий");
        ui->add->setText("Следующий");
        ui->stackedWidget->setCurrentIndex(0);
//        ui->slider->setMaximum(pics.size() - 1);
        draw(0);
        show_text(0);
//        for (auto record : record_items) {
//            ui->view_grid->addWidget(record);
//        }
        break;
    case RELEASE_PREPARATION:
        load_hashtag_info();
        read_poll_logs();
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
    case TITLES:
        if (current_mode == JOURNAL_READING || current_mode == RELEASE_PREPARATION) {
            ui->stacked_view->setCurrentIndex(3);
            lay_titles();
        }
        break;
    }
    ui->main_view->setChecked(current_view == MAIN);
    ui->list_view->setChecked(current_view == LIST);
    ui->gallery_view->setChecked(current_view == GALLERY);
    ui->preview_view->setChecked(current_view == PREVIEW);
    ui->title_view->setChecked(current_view == TITLES);
}

void MainWindow::lay_previews(int page) {
    if (current_view == MAIN || current_mode == IDLE) return;
    int total_previews = current_mode == JOURNAL_READING
                            ? (filtration_results.isEmpty()
                                ? records.size()
                                : filtration_results.values().size())
                            : record_items.size();
    ui->page_index->setMaximum(total_previews / pics_per_page + 1);
    clear_grid(ui->view_grid);
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

void MainWindow::lay_titles() {
//    int total_titles = title_map.size();
//    ui->page_index->setMaximum(total_previews / pics_per_page + 1);
//    clear_view_grid();
    for (int i = 0 ; i < title_items.size(); ++i) {
//        title_items[i]->set_gallery_view();
        ui->title_grid->addWidget(title_items[i], i/9, i%9);
    }
}

void MainWindow::check_titles(bool enable) {
    for (auto item : title_items) {
        dynamic_cast<RecordTitleItem*>(item)->set_checked(enable);
    }
}

void MainWindow::set_enabled(bool enable) {
    ui->back->setEnabled(enable && pic_index > 0 && current_mode != RELEASE_PREPARATION);
    ui->ok->setEnabled(enable && current_mode != RELEASE_PREPARATION);
    ui->skip->setEnabled(enable && current_mode == JOURNAL_CREATION);
    bool listing_enabled = (current_mode == JOURNAL_CREATION && pics.size() > 1)
            || (current_mode == JOURNAL_READING && records[0].pics.size() > 1);
    ui->add->setEnabled(enable && listing_enabled);
    ui->text->setEnabled(enable && current_mode != TEXT_READING && current_mode != RELEASE_PREPARATION);
    ui->private_switch->setEnabled(current_mode == JOURNAL_CREATION || current_mode == JOURNAL_READING);
    ui->load_subs->setEnabled(current_mode == TEXT_READING);
    ui->slider->setEnabled(current_mode == JOURNAL_READING);
    ui->slider->setValue(0);
    ui->preview_view->setEnabled(current_mode == RELEASE_PREPARATION);
    ui->poll_preparation->setEnabled(current_mode == RELEASE_PREPARATION);
    ui->title_view->setEnabled(current_mode == JOURNAL_READING || current_mode == RELEASE_PREPARATION);
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
        save_title_journal(start, end);
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
    auto json_file = json_object(locations[JOURNALS] + "\\result\\titles.json");
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
    case JOURNAL_CREATION: {
        bool multiple_pics = pic_end_index > 0;
        QString s_quote = QString().setNum(quote_index + 1) + " из " + QString().setNum(quotes.size());
        QString s_pic = multiple_pics ? "кадры " : "кадр ";
        QString s_pic_index = QString().setNum(pic_index + 1) + (multiple_pics ? "-" + QString().setNum(pic_end_index + 1) : "");
        QString s_pic_from = " из " + QString().setNum(pics.size());
        ui->statusBar->showMessage("Цитата " + s_quote + ", " + s_pic + s_pic_index + s_pic_from);
        break;
    }
    case JOURNAL_READING: {
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
