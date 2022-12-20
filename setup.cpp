#include "mainwindow.h"
#include "ui_mainwindow.h"
#include <QDebug>

MainWindow::MainWindow(QWidget *parent) :
    QMainWindow(parent),
    ui(new Ui::MainWindow),
    manager(new VK_Manager())
{
    ui->setupUi(this);
    initialize();
    get_hashtags();

    connect(manager, &VK_Manager::albums_ready, [this](const QMap<QString, int>& ids) {
        album_ids = ids;
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

    connect(ui->open_folder, &QAction::triggered, [this]() {
        clear_all();
        dir = QDir(QFileDialog::getExistingDirectory(nullptr, "Открыть папку с кадрами",
                                                     screenshots_location));
        pics = dir.entryList(QDir::Files | QDir::NoDotAndDotDot);
        QFile file(quotes_location + dir.dirName() + ".txt");
        if (!read_quote_file(file)) {
            clear_all();
            return;
        }
        manager->get_photo_ids(album_ids[dir.dirName()]);
    });

    connect(ui->open_config, &QAction::triggered, [this]() {
        if (!open_title_config()) {
            set_mode(IDLE);
            ui->statusBar->showMessage("Конфигурационный файл не открыт.");
            return;
        }
        set_mode(CONFIG_READING);
    });

    connect(ui->compile, &QAction::triggered, this, &MainWindow::compile_configs);
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

    connect(ui->slider, &QAbstractSlider::valueChanged, [this](int value) {
        switch (current_mode) {
        case CONFIG_READING:
            pic_end_index = 0;
            pic_index = value;
            display(pic_index);
            update_current_hashtags();
            break;
        default:
            break;
        }
        show_status();
    });
//    connect(ui->refactor, &QAction::triggered, this, &MainWindow::refactor_configs);

    connect(ui->main_view, &QAction::triggered, [this]() { set_view(MAIN); });
    connect(ui->list_view, &QAction::triggered, [this]() { set_view(LIST); });
    connect(ui->gallery_view, &QAction::triggered, [this]() { set_view(GALLERY); });

    connect(ui->alphabet_order, &QAction::triggered, [this]() {
        ui->addition_order->setChecked(false);
        update_hashtag_grid();
    });
    connect(ui->addition_order, &QAction::triggered, [this]() {
        ui->alphabet_order->setChecked(false);
        update_hashtag_grid();
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
            update_quote_file();
            save_title_config();
            break;
        default:
            break;
        }
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
            ui->slider->setValue(current_index);
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
        default:
            break;
        }
        show_status();
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
            ui->make_private->setChecked(false);
            if (pic_index < pics.size()) {
                draw(pic_index);
                ui->slider->setValue(pic_index);
                show_text(++quote_index);
            } else {
                save_title_config();
                update_quote_file();
                set_mode(IDLE);
            }
            break;
        case CONFIG_READING:
            if (record_edited) {
                update_record();
                if (pic_index + 1 == records.size()) {
                    ui->ok->setEnabled(false);
                    ui->skip->setEnabled(true);
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
        ui->stackedWidget->setCurrentIndex(1);
    }
}

void MainWindow::keyReleaseEvent(QKeyEvent* event) {
    switch (event->key()) {
    case Qt::Key_Control:
        ui->stackedWidget->setCurrentIndex(0);
        break;
    case Qt::Key_F1:
        set_view(MAIN);
        break;
    case Qt::Key_F2:
        set_view(LIST);
        break;
    case Qt::Key_F3:
        set_view(GALLERY);
        break;
    case Qt::Key_1:
        emit hashtags["тема_1_лицо"]->hashtagEvent('#',"тема_1_лицо");
        break;
    case Qt::Key_2:
        emit hashtags["тема_2_лицо"]->hashtagEvent('#',"тема_2_лицо");
        break;
    case Qt::Key_3:
        emit hashtags["тема_3_лицо"]->hashtagEvent('#',"тема_3_лицо");
        break;
    case Qt::Key_4:
        emit hashtags["тема_безличная"]->hashtagEvent('#',"тема_безличная");
        break;
    case Qt::Key_5:
        emit hashtags["тема_объект"]->hashtagEvent('#',"тема_объект");
        break;
    case Qt::Key_Enter:
        emit ui->ok->clicked();
        break;
    case Qt::Key_Left:
        emit ui->back->clicked();
        break;
    default:
        break;
    }
}

void MainWindow::initialize() {
    auto json_file = json_object("config.json");
    if (!json_file.contains("screenshots") || !json_file.contains("docs") || !json_file.contains("configs")) {
        ui->statusBar->showMessage("Неверный формат конфигурационного файла.");
        return;
    }
    screenshots_location = json_file.value("screenshots").toString();
    quotes_location = json_file.value("docs").toString();
    configs_location = json_file.value("configs").toString();
    access_token = json_file.value("access_token").toString();
    client_id = json_file.value("client").toInt();
    manager->set_access_token(access_token);
    manager->get_albums();
    if (!QDir(screenshots_location).exists() || !QDir(quotes_location).exists()) {
        screenshots_location = QString();
        quotes_location = QString();
        ui->statusBar->showMessage("Указаны несуществующие директории. Перепроверьте конфигурационный файл.");
        return;
    }
    ui->statusBar->showMessage("Конфигурация успешно загружена.");
}

void MainWindow::clear_all() {
    quotes.clear();
    photo_ids.clear();
    pics.clear();
    links.clear();
    records.clear();
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
        ui->make_private->setText("Скрыть");
        ui->slider->setMaximum(pics.size() - 1);
        draw(0);
        break;
    case CONFIG_READING:
        ui->ok->setText("Далее");
        ui->add->setText("Листать");
        ui->skip->setText("Сохранить");
        ui->make_private->setText("Скрыто");
        ui->slider->setMaximum(records.size() - 1);
        connect(ui->make_private, &QPushButton::clicked, this, &MainWindow::set_edited);
        display(0);
        load_hashtag_info();
        for (auto record : record_items) {
            ui->view_grid->addWidget(record);
        }
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
        break;
    case LIST: case GALLERY:
        ui->stacked_view->setCurrentIndex(1);
        QLayoutItem* child;
        while ((child = ui->view_grid->takeAt(0))) {
            // Clearing items from the grid
            child->widget()->hide();
        }
        const auto& items = filtration_results.empty()
                ? record_items
                : filtration_results.values();
        for (int i = 0; i < items.size(); ++i) {
            if (view == LIST) {
                items[i]->set_list_view();
                ui->view_grid->addWidget(items[i], i, 0);
            } else {
                items[i]->set_gallery_view();
                ui->view_grid->addWidget(items[i], i/5, i%5);
            }
        }
        break;
    }
    ui->main_view->setChecked(current_view == MAIN);
    ui->list_view->setChecked(current_view == LIST);
    ui->gallery_view->setChecked(current_view == GALLERY);
}

void MainWindow::set_enabled(bool enable) {
    ui->back->setEnabled(enable && pic_index > 0);
    ui->ok->setEnabled(enable);
    ui->skip->setEnabled(enable && current_mode == CONFIG_CREATION);
    bool listing_enabled = (current_mode == CONFIG_CREATION && pics.size() > 1)
            || (current_mode == CONFIG_READING && records[0].pics.size() > 1);
    ui->add->setEnabled(enable && listing_enabled);
    ui->text->setEnabled(enable);
    ui->make_private->setEnabled(enable);
    ui->slider->setEnabled(current_mode == CONFIG_READING);
    ui->slider->setValue(0);
}

void MainWindow::set_edited() {
    record_edited = true;
    config_edited = true;
    ui->ok->setEnabled(true);
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
    if (current_mode == CONFIG_CREATION) {
        bool multiple_pics = pic_end_index > 0;
        QString s_quote = QString().setNum(quote_index + 1) + " из " + QString().setNum(quotes.size());
        QString s_pic = multiple_pics ? "кадры " : "кадр ";
        QString s_pic_index = QString().setNum(pic_index + 1) + (multiple_pics ? "-" + QString().setNum(pic_end_index + 1) : "");
        QString s_pic_from = " из " + QString().setNum(pics.size());
        ui->statusBar->showMessage("Цитата " + s_quote + ", " + s_pic + s_pic_index + s_pic_from);
    } else if (current_mode == CONFIG_READING) {
        QString filtration = ui->text->isEnabled()
                ? ""
                : filtration_message(filtration_results.size()) + filtration_indices();
        QString s_rec = QString().setNum(pic_index + 1) + " из " + QString().setNum(records.size());
        QString s_pic = QString().setNum(pic_end_index + 1) + " из " + QString().setNum(records[pic_index].pics.size());
        ui->statusBar->showMessage(filtration + "Запись " + s_rec + ", кадр " + s_pic);
    }
}
