#include "mainwindow.h"
#include "ui_mainwindow.h"
#include <QDebug>

MainWindow::MainWindow(QWidget *parent) :
    QMainWindow(parent),
    ui(new Ui::MainWindow),
    manager(new VK_Manager())
{
    ui->setupUi(this);
    load();
//    load_albums();

    connect(manager, &VK_Manager::albums_ready, [this](QNetworkReply *response) {
        disconnect(manager, &QNetworkAccessManager::finished, manager, &VK_Manager::albums_ready);
        if (!load_albums(reply(response))) {
            manager->get_access_token(client_id);
            manager->get_albums();
            update_config();
        }
//        save_albums(this->reply(response));
    });

    connect(manager, &VK_Manager::photos_ready, [this](QNetworkReply *response) {
//        get_urls(this->reply(response));
        get_ids(reply(response));
        set_enabled(initialization_status());
        disconnect(manager, &QNetworkAccessManager::finished, manager, &VK_Manager::photos_ready);
    });

    connect(manager, &VK_Manager::image_ready, [this](QNetworkReply *response) {
        ui->image->setPixmap(scaled(this->image(response)));
    });

    connect(ui->open_folder, &QAction::triggered, [this]() {
        clear_all();
        dir = QDir(QFileDialog::getExistingDirectory(nullptr, "Открыть папку с кадрами",
                                                     screenshots_location));
        pics = dir.entryList(QDir::Files | QDir::NoDotAndDotDot);
        QFile file(quotes_location + dir.dirName() + ".txt");
        read_quote_file(file);
        manager->get_photos(album_ids[dir.dirName()]);
    });

    connect(ui->open_doc, &QAction::triggered, [this]() {
        if (ui->ok->isEnabled()) {
            pics.clear();
        }
        quotes.clear();
        auto filepath = QFileDialog::getOpenFileName(nullptr, "Открыть файл с цитатами",
                                                     quotes_location,
                                                     "Текстовые документы (*.txt)");
        QFile file(filepath);
        read_quote_file(file);
        show_status();
    });

    connect(ui->open_config, &QAction::triggered, [this]() {
        if (!open_json()) {
            return;
        }
        set_mode(CONFIG_READING);
        display(0);
        show_status();
    });

    connect(ui->skip, &QPushButton::clicked, [this]() {
        pic_index = qMax(pic_index, pic_end_index) + 1;
        draw(pic_index);
        show_status();
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
        }
            break;
        case CONFIG_READING:
            pic_end_index = 0;
            display(--pic_index);
            break;
        default:
            break;
        }
        show_status();
    });

    connect(ui->add, &QPushButton::clicked, [this]() {
        switch (current_mode) {
        case CONFIG_CREATION:
            pic_end_index = qMax(pic_index, pic_end_index) + 1;
            draw(pic_end_index);
            break;
        case CONFIG_READING:
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
                show_text(++quote_index);
            } else {
                save_title_config();
                update_quote_file();
                set_mode(IDLE);
            }
            break;
        case CONFIG_READING:
            pic_end_index = 0;
            display(++pic_index);
            break;
        default:
            break;
        }
        show_status();
    });
}

MainWindow::~MainWindow() {
    delete ui;
}

void MainWindow::clear_all() {
    quotes.clear();
    photo_ids.clear();
    pics.clear();
    records.clear();
}

QJsonObject MainWindow::reply(QNetworkReply *response) {
    response->deleteLater();
    if (response->error() != QNetworkReply::NoError) return QJsonObject();
    auto reply = QJsonDocument::fromJson(response->readAll()).object();
    return reply;
}

QImage MainWindow::image(QNetworkReply *response) {
    QImageReader reader(response);
    QImage loaded_image = reader.read();
    return loaded_image;
}

void MainWindow::get_urls(const QJsonObject & reply) {
    auto array = reply["response"].toObject()["items"].toArray();
    for (const QJsonValueRef item : array) {
        auto url = item.toObject()["sizes"].toArray().last().toObject()["url"].toString();
        urls.push_back(url);
    }
}

void MainWindow::get_ids(const QJsonObject & reply) {
    auto array = reply["response"].toObject()["items"].toArray();
    for (const QJsonValueRef item : array) {
        auto id = item.toObject()["id"].toInt();
        photo_ids.push_back(id);
    }
}

bool MainWindow::open_json() {
    clear_all();
    auto filepath = QFileDialog::getOpenFileName(nullptr, "Открыть json-файл",
                                                 configs_location,
                                                 "Файлы (*.json)");
    auto json_file = json_object(filepath);
    if (!json_file.contains("title") || !json_file.contains("screens")) {
        ui->statusBar->showMessage("Неверный формат файла.");
        return false;
    }
    read_title_config(json_file);
    return !records.empty();
}

void MainWindow::save_albums(const QJsonObject& reply) {
    QFile file("albums.json");
    QJsonObject albums_config;
    auto items = reply["response"].toObject()["items"].toArray();
    for (const QJsonValueRef album : items) {
        auto album_object = album.toObject();
        auto title = album_object["title"].toString();
        int album_id = album_object["id"].toInt();
        albums_config[title] = album_id;
    }
    auto message = save_json(albums_config, file) ? "Список альбомов сохранён." : "Не удалось сохранить список альбомов.";
    ui->statusBar->showMessage(message);
}

bool MainWindow::load_albums() {
    auto json_file = json_object("albums.json");
    for (const auto& key : json_file.keys()) {
        album_ids.insert(key, json_file[key].toInt());
    }
    return true;
}

bool MainWindow::load_albums(const QJsonObject& reply) {
    QJsonObject albums_config;
    if (reply.contains("error")) {
        return false;
    }
    auto items = reply["response"].toObject()["items"].toArray();
    for (const QJsonValueRef album : items) {
        auto album_object = album.toObject();
        auto title = album_object["title"].toString();
        int album_id = album_object["id"].toInt();
        album_ids.insert(title, album_id);
    }
    return true;
}

void MainWindow::set_mode(Mode mode) {
    current_mode = mode;
    quote_index = pic_index = pic_end_index = 0;
    switch (mode) {
    case CONFIG_CREATION:
        ui->ok->setText("Готово");
        ui->add->setText("Добавить");
        draw(0);
        break;
    case CONFIG_READING:
        ui->ok->setText("Далее");
        ui->add->setText("Листать");
        break;
    default:
        break;
    }
    set_enabled(mode);
}

void MainWindow::load() {
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

void MainWindow::update_config() {
    auto config_path = "config.json";
    auto json_file = json_object(config_path);
    json_file["access_token"] = manager->current_token();
    QFile config(config_path);
    save_json(json_file, config);
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

void MainWindow::register_record() {
    Record record;
    for (int i = pic_index; i <= qMax(pic_index, pic_end_index); ++i) {
        record.pics.push_back(pics[i]);
        record.ids.push_back(photo_ids[i]);
    }
    record.quote = ui->text->toPlainText();
    record.is_public = !ui->make_private->isChecked();
    records.push_back(record);
}

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
        set_enabled(!pics.empty());
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

void MainWindow::read_title_config(const QJsonObject& json_file) {
    auto title = json_file.value("title").toString();
    dir = QDir(screenshots_location + title);
    auto records_array = json_file.value("screens").toArray();

    for (QJsonValueRef r : records_array) {
        Record record;
        auto object = r.toObject();
        record.quote = object["caption"].toString();
        record.is_public = object["public"].toBool();
        auto filename_array = object["filenames"].toArray();
        for (QJsonValueRef name : filename_array) {
            record.pics.push_back(name.toString());
        }
        auto id_array = object["photo_ids"].toArray();
        for (QJsonValueRef id : id_array) {
            record.ids.push_back(id.toInt());
        }
        records.push_back(record);
    }
}

QJsonObject MainWindow::Record::to_json() const {
    QJsonObject current_record;
    QJsonArray pic_array, id_array;
    for (int i = 0; i < pics.size(); ++i) {
        pic_array.push_back(pics[i]);
        id_array.push_back(ids[i]);
    }
    current_record["filenames"] = pic_array;
    current_record["photo_ids"] = id_array;
    current_record["caption"] = quote;
    current_record["public"] = is_public;
    return current_record;
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
    auto message = save_json(object, file) ? "Конфигурационный файл сохранён." : "Не удалось сохранить файл.";
    ui->statusBar->showMessage(message);
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

void MainWindow::set_enabled(bool enable) {
    ui->back->setEnabled(enable);
    ui->ok->setEnabled(enable);
    ui->skip->setEnabled(enable && current_mode == CONFIG_CREATION);
    ui->add->setEnabled(enable);
    ui->text->setEnabled(enable);
    ui->make_private->setEnabled(enable && current_mode == CONFIG_CREATION);
}

QPixmap MainWindow::scaled(const QImage& source) {
    return QPixmap::fromImage(source.scaled(ui->image->geometry().size(), Qt::KeepAspectRatio));
}

void MainWindow::display(int index) {
    if (current_mode == CONFIG_READING) {
        auto image = QImage(dir.path() + QDir::separator() + records[index].pics[pic_end_index]);
        ui->image->setPixmap(scaled(image));
    }
    ui->text->setText(records[index].quote);
    bool reached_end = index + 1 >= records.size();
    bool listing_on = pic_end_index + 1 < records[index].pics.size();
    ui->add->setEnabled(listing_on && !reached_end);
    ui->ok->setEnabled(!reached_end);
    ui->back->setEnabled(index > 0);
}

void MainWindow::draw(int index = 0) {
    auto image = QImage(dir.path() + QDir::separator() + pics[index]);
    ui->image->setPixmap(scaled(image));
    bool reached_end = index + 1 >= pics.size();
    ui->skip->setEnabled(!reached_end);
    ui->add->setEnabled(!reached_end);
    ui->back->setEnabled(index > 0);
}

void MainWindow::show_text(int index) {
    if (quotes.size() <= index) return;
    ui->text->setText(quotes[index]);
}

bool MainWindow::initialization_status() {
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
    ui->statusBar->showMessage("Загружено кадров: " + QString().setNum(pics.size()) + ", цитат: " + QString().setNum(quotes.size()) + ".");
    set_mode(CONFIG_CREATION);
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
