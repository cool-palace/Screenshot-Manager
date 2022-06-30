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

    connect(manager, &VK_Manager::albums_ready, [this](QNetworkReply *response) {
        disconnect(manager, &QNetworkAccessManager::finished, manager, &VK_Manager::albums_ready);
        load_albums(reply(response));
    });

    connect(manager, &VK_Manager::photos_ready, [this](QNetworkReply *response) {
        get_ids(reply(response));
        set_mode(data_ready() ? CONFIG_CREATION : IDLE);
        disconnect(manager, &QNetworkAccessManager::finished, manager, &VK_Manager::photos_ready);
    });

    connect(manager, &VK_Manager::photo_ready, [this](QNetworkReply *response) {
        get_link(reply(response));
        disconnect(manager, &QNetworkAccessManager::finished, manager, &VK_Manager::photo_ready);
    });

    connect(manager, &VK_Manager::image_ready, [this](QNetworkReply *response) {
        ui->image->setPixmap(scaled(image(response)));
        disconnect(manager, &QNetworkAccessManager::finished, manager, &VK_Manager::image_ready);
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

    connect(ui->open_config, &QAction::triggered, [this]() {
        if (!open_title_config()) {
            return;
        }
        set_mode(CONFIG_READING);
        show_status();
    });

    connect(ui->compile, &QAction::triggered, this, &MainWindow::compile_configs);

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
    delete manager;
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
    records.clear();
}

void MainWindow::set_mode(Mode mode) {
    current_mode = mode;
    quote_index = pic_index = pic_end_index = 0;
    switch (mode) {
    case CONFIG_CREATION:
        ui->ok->setText("Готово");
        ui->add->setText("Добавить");
        ui->make_private->setText("Скрыть");
        draw(0);
        break;
    case CONFIG_READING:
        ui->ok->setText("Далее");
        ui->add->setText("Листать");
        ui->make_private->setText("Скрыто");
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

void MainWindow::get_link(const QJsonObject & reply) {
    disconnect(manager, &QNetworkAccessManager::finished, manager, &VK_Manager::photo_ready);
    auto url = reply["response"].toArray().first().toObject()["sizes"].toArray().last().toObject()["src"].toString();
    manager->get_url(url);
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
