#include "mainwindow.h"
#include "ui_mainwindow.h"
#include <QDebug>

MainWindow::MainWindow(QWidget *parent) :
    QMainWindow(parent),
    ui(new Ui::MainWindow),
    manager(new VK_Manager())
{
    ui->setupUi(this);
    add_tag_button = new QPushButton("+");
    add_tag_button->setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Expanding);
    connect(add_tag_button, &QPushButton::clicked, [this]() {
        bool ok;
        QString text = QInputDialog::getText(this, tr("Добавление хэштега"),
                                                   tr("Введите новый хэштег:"), QLineEdit::Normal,
                                                   "", &ok);
        if (ok && !text.isEmpty() && !hashtags.contains(text)) {
            create_hashtag_button(text);
            update_hashtag_grid();
        }
    });
    get_hashtags();
    initialize();

    connect(manager, &VK_Manager::albums_ready, [this](QNetworkReply *response) {
        disconnect(manager, &QNetworkAccessManager::finished, manager, &VK_Manager::albums_ready);
        load_albums(reply(response));
        if (album_ids.empty()) {
//            manager->get_access_token(client_id);
            ui->statusBar->showMessage("Не удалось загрузить альбомы. Попробуйте авторизироваться вручную.");
        }
    });

    connect(manager, &VK_Manager::photo_ids_ready, [this](QNetworkReply *response) {
        disconnect(manager, &QNetworkAccessManager::finished, manager, &VK_Manager::photo_ids_ready);
        get_ids(reply(response));
        set_mode(data_ready() ? CONFIG_CREATION : IDLE);
    });

//    connect(manager, &VK_Manager::photo_ready, [this](QNetworkReply *response) {
//        disconnect(manager, &QNetworkAccessManager::finished, manager, &VK_Manager::photo_ready);
//        get_link(reply(response));
//    });

    connect(manager, &VK_Manager::image_ready, [this](QNetworkReply *response) {
        disconnect(manager, &QNetworkAccessManager::finished, manager, &VK_Manager::image_ready);
        ui->image->setPixmap(scaled(image(response)));
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
            return;
        }
        set_mode(CONFIG_READING);
        show_status();
    });

    connect(ui->compile, &QAction::triggered, this, &MainWindow::compile_configs);
    connect(ui->slider, &QAbstractSlider::valueChanged, [this](int value) {
        switch (current_mode) {
        case CONFIG_READING:
            pic_end_index = 0;
            pic_index = value;
            display(pic_index);
            break;
        default:
            break;
        }
        show_status();
    });
//    connect(ui->refactor, &QAction::triggered, this, &MainWindow::refactor_configs);

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
            display(--pic_index);
            ui->slider->setValue(pic_index);
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
            display(++pic_index);
            ui->slider->setValue(pic_index);
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
    if (event->key() == Qt::Key_Control) {
        ui->stackedWidget->setCurrentIndex(0);
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

void MainWindow::get_hashtags() {
    QFile file("hashtags.txt");
    if (!file.open(QIODevice::ReadOnly)) {
        ui->statusBar->showMessage("Не удалось открыть файл с хэштегами.");
    }
    QTextStream in(&file);
    in.setCodec("UTF-8");
    while (!in.atEnd()) {
        QString tag = in.readLine();
        if (tag[0] == '#') {
            tag.remove(0,1);
        }
        create_hashtag_button(tag);
    }
    file.close();
    update_hashtag_grid();
}

void MainWindow::create_hashtag_button(const QString& text) {
    hashtags.insert(text, new QPushButton(text));
    hashtags[text]->setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Expanding);
    connect(hashtags[text], &QPushButton::clicked, [this, text]() {
        if (current_mode == IDLE) return;
        QRegularExpression regex("#" + text);
        QRegularExpressionMatchIterator i = regex.globalMatch(ui->text->toPlainText());
        if (!i.hasNext()) {
            ui->text->setText(ui->text->toPlainText() + " #" + text);
        }
    });
}

void MainWindow::update_hashtag_grid() {
    while (ui->tag_grid->takeAt(0) != nullptr) {
        // Clearing buttons from the grid
    }
    int i = 0;
    for (const auto& button : hashtags) {
        ui->tag_grid->addWidget(button, i / 10, i % 10);
        ++i;
    }
    ui->tag_grid->addWidget(add_tag_button, i / 10, i % 10);
}

void MainWindow::clear_all() {
    quotes.clear();
    photo_ids.clear();
    pics.clear();
    records.clear();
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
        break;
    default:
        break;
    }
    set_enabled(mode);
    show_status();
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

QPixmap MainWindow::scaled(const QImage& source) {
    return QPixmap::fromImage(source.scaled(ui->image->geometry().size(), Qt::KeepAspectRatio));
}

QJsonObject MainWindow::json_object(const QString& filepath) {
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

bool MainWindow::save_json(const QJsonObject& object, QFile& file) {
    if (!file.open(QIODevice::WriteOnly | QIODevice::Truncate)) {
        return false;
    }
    QTextStream out(&file);
    out.setCodec("UTF-8");
    out << QJsonDocument(object).toJson();
    file.close();
    return true;
}

QJsonObject MainWindow::reply(QNetworkReply *response) {
    response->deleteLater();
    if (response->error() != QNetworkReply::NoError) return QJsonObject();
    auto reply = QJsonDocument::fromJson(response->readAll()).object();
    return reply;
}

QString MainWindow::link(const QJsonObject & photo_item) {
    auto array = photo_item["sizes"].toArray();
    QString url;
    for (QJsonValueRef item : array) {
        if (item.toObject()["type"].toString() == "z") {
            url = item.toObject()["url"].toString();
            break;
        }
    }
    if (url.isEmpty()) {
        for (QJsonValueRef item : array) {
            if (item.toObject()["type"].toString() == "y") {
                url = item.toObject()["url"].toString();
                break;
            }
        }
    }
    return url;
}

QImage MainWindow::image(QNetworkReply *response) {
    QImageReader reader(response);
    QImage loaded_image = reader.read();
    return loaded_image;
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

void MainWindow::show_status() {
    if (current_mode == CONFIG_CREATION) {
        bool multiple_pics = pic_end_index > 0;
        QString s_quote = QString().setNum(quote_index + 1) + " из " + QString().setNum(quotes.size());
        QString s_pic = multiple_pics ? "кадры " : "кадр ";
        QString s_pic_index = QString().setNum(pic_index + 1) + (multiple_pics ? "-" + QString().setNum(pic_end_index + 1) : "");
        QString s_pic_from = " из " + QString().setNum(pics.size());
        ui->statusBar->showMessage("Цитата " + s_quote + ", " + s_pic + s_pic_index + s_pic_from);
    } else if (current_mode == CONFIG_READING) {
        QString s_rec = QString().setNum(pic_index + 1) + " из " + QString().setNum(records.size());
        QString s_pic = QString().setNum(pic_end_index + 1) + " из " + QString().setNum(records[pic_index].pics.size());
        ui->statusBar->showMessage("Запись " + s_rec + ", кадр " + s_pic);
    }
}
