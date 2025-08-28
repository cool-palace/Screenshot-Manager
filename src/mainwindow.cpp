#include "include\mainwindow.h"
#include "ui_mainwindow.h"
#include <QDebug>
#include "include\journal_creation.h"
#include "include\text_reading.h"
#include "include\journal_reading.h"
#include "include\release_preparation.h"

MainWindow::MainWindow(QWidget *parent)
    : QMainWindow(parent)
    , ui(new Ui::MainWindow)
{
    ui->setupUi(this);
    initialize();
    ui->stacked_view->setCurrentIndex(0);
    ui->toolBar->hide();

    connect(ui->journal_creation, &QAction::triggered, this, &MainWindow::journal_creation);
    connect(ui->journal_reading, &QAction::triggered, this, &MainWindow::journal_reading);
    connect(ui->journal_reading_all, &QAction::triggered, this, &MainWindow::journal_reading_all);
    connect(ui->text_reading, &QAction::triggered, this, &MainWindow::text_reading);
    connect(ui->descriptions_reading, &QAction::triggered, this, &MainWindow::descriptions_reading);
    connect(ui->release_preparation, &QAction::triggered, this, &MainWindow::release_preparation);
    connect(ui->text_labeling, &QAction::triggered, this, &MainWindow::text_labeling);

    connect(ui->journal_creation_button, &QPushButton::clicked, this, &MainWindow::journal_creation);
    connect(ui->journal_reading_button, &QPushButton::clicked, this, &MainWindow::journal_reading);
    connect(ui->text_reading_button, &QPushButton::clicked, this, &MainWindow::text_reading);
    connect(ui->release_preparation_db_button, &QPushButton::clicked, this, &MainWindow::release_preparation_db);
    connect(ui->poll_preparation_button, &QPushButton::clicked, this, &MainWindow::poll_preparation_db);
    connect(ui->text_labeling_button, &QPushButton::clicked, this, &MainWindow::text_labeling);
    connect(ui->exit_mode, &QAction::triggered, this, &MainWindow::exit_mode);
    connect(ui->initialization, &QAction::triggered, this, &MainWindow::initialize);
    connect(ui->compile, &QAction::triggered, this, &MainWindow::compile_journals);
    connect(ui->compile_to_db, &QAction::triggered, [this]() {
        progress_dialog = new ProgressDialog();
        progress_dialog->show();
        QtConcurrent::run(this, &MainWindow::compile_journals_to_db);
    });
    connect(ui->export_text, &QAction::triggered, this, &MainWindow::export_text);
//    connect(ui->add_hashtag, &QAction::triggered, this, &MainWindow::add_hashtag);

}

MainWindow::~MainWindow() {
    delete ui;
}

void MainWindow::keyPressEvent(QKeyEvent* event) {
    emit key_press(event);
}

void MainWindow::keyReleaseEvent(QKeyEvent* event) {
    emit key_release(event);
}

void MainWindow::exit_mode() {
    ui->stacked_view->setCurrentIndex(0);
    ui->exit_mode->setEnabled(false);
    ui->statusBar->clearMessage();
    ui->toolBar->hide();
}

void MainWindow::resizeEvent(QResizeEvent *event) {
    QMainWindow::resizeEvent(event);
    auto pic = ui->label->pixmap();
    if (pic) {
        ui->image->setPixmap(pic->scaled(ui->image->size(), Qt::KeepAspectRatio, Qt::FastTransformation));
    }
    auto operation_mode = dynamic_cast<AbstractOperationMode*>(mode);
    if (operation_mode) {
        operation_mode->resize_event(event);
    }
}

void MainWindow::closeEvent(QCloseEvent *event) {
    emit close_event(event);
}

bool MainWindow::initialize() {
    auto json_file = json_object("config.json");
    if (json_file.empty()) {
        auto config_file = QFileDialog::getOpenFileName(nullptr, "Открыть конфигурационный файл",
                                                                 locations[JOURNALS],
                                                                 "Файлы (*.json)");
        json_file = json_object(config_file);
    }
    if (!json_file.contains("screenshots") /*|| !json_file.contains("docs")*/ || !json_file.contains("configs")) {
        ui->statusBar->showMessage("Неверный формат конфигурационного файла.");
        return false;
    }
    Locations::instance().init(json_file);

    QString access_token = json_file.value("access_token").toString();
    QString group_id = json_file.value("group_id").toString();
    QString public_id = json_file.value("public_id").toString();
    VK_Manager::instance().init(access_token, group_id, public_id);
    if (!QDir(Locations::instance()[SCREENSHOTS]).exists() || !QDir(Locations::instance()[JOURNALS]).exists()) {
        ui->statusBar->showMessage("Указаны несуществующие директории. Перепроверьте конфигурационный файл.");
        return false;
    }
    ui->statusBar->showMessage("Конфигурация успешно загружена.");
    return true;
}

void MainWindow::journal_creation() {
    if (mode) {
        qDebug() << mode;
        mode->~AbstractMode();
        mode = new(mode) JournalCreation(this);
    } else mode = new JournalCreation(this);
    qDebug() << mode;
    mode->start();
}

void MainWindow::journal_reading() {
    if (mode) {
        qDebug() << mode;
        mode->~AbstractMode();
        mode = new(mode) JournalReading(this);
    } else mode = new JournalReading(this);
    qDebug() << mode;
    mode->start();
}

void MainWindow::journal_reading_all() {
    if (mode) {
        qDebug() << mode;
        mode->~AbstractMode();
        mode = new(mode) JournalReading(this, true);
    } else mode = new JournalReading(this, true);
    qDebug() << mode;
    mode->start();
}

void MainWindow::text_reading() {
    if (mode) {
        qDebug() << mode;
        mode->~AbstractMode();
        mode = new(mode) TextReading(this);
    } else mode = new TextReading(this);
    qDebug() << mode;
    mode->start();
}

void MainWindow::descriptions_reading() {
//    clear_all();
//    auto path = QFileDialog::getOpenFileName(nullptr, "Открыть файл с описаниями",
//                                                       locations[JOURNALS] + "//descriptions",
//                                                       "Файлы (*.json)");
//    read_descriptions(json_object(path));
//    if (records.isEmpty()/* quotes.isEmpty() || pics.isEmpty()*/) {
//        set_mode(IDLE);
//        ui->statusBar->showMessage("Не удалось прочесть описания из файла.");
//        return;
//    }
//    set_mode(DESCRIPTION_READING);
//    set_view(MAIN);
}

void MainWindow::release_preparation() {
    if (mode) {
        qDebug() << mode;
        mode->~AbstractMode();
        mode = new(mode) ReleasePreparation(this);
    } else mode = new ReleasePreparation(this);
    qDebug() << mode;
    mode->start();
}

void MainWindow::release_preparation_db() {
    ui->stacked_view->setCurrentIndex(6);
    ui->wReleasePreparation->start();
}

void MainWindow::poll_preparation_db() {
    ui->stacked_view->setCurrentIndex(7);
    ui->wPollPreparation->start();
}

void MainWindow::text_labeling() {
    ui->stacked_view->setCurrentIndex(5);
    ui->labeling_widget->start();
}

void MainWindow::read_descriptions(const QJsonObject& json_file) {
//    auto info_json = json_object(locations[JOURNALS] + "result\\photo_info.json");
//    for (auto it = json_file.begin(); it != json_file.end(); ++it) {
//        auto info = info_json.value(it.key()).toObject();
//        Record record;
//        record.ids.append(it.key().toInt());
//        auto path = info.value("path").toString().split('\\');
////        qDebug() << path;
//        auto title = path.first();
//        if (title_map.isEmpty() || title_map.last() != title) {
////            title_map[quotes.size()] = title;
//            title_map[records.size()] = title;
//        }
//        record.pics.append(path.back());
//        record.links.append(info["link"].toString());
//        record.quote = it.value().toString();
//        record.is_public = true;
//        records.append(record);
////        photo_ids.append(it.key().toInt());
////        links.append(info["link"].toString());
////        pics.append(path.back());
////        quotes.append(it.value().toString());
//    }
////    qDebug() << records.size();
////    qDebug() << records.first().quote << records.first().pics << records.first().links;
//    qDebug() << title_map;
}

void MainWindow::compile_journals() {
    QDir dir = QDir(locations[JOURNALS]);
    auto configs = dir.entryList(QDir::Files | QDir::NoDotAndDotDot);
    if (configs.contains(".test.json")) {
        configs.removeAt(configs.indexOf(".test.json"));
    }
    QJsonArray resulting_array;
    QJsonArray hidden_array;
    QJsonObject title_map;
    QJsonObject series_map;
    for (const auto& config : configs) {
        auto object = json_object(locations[JOURNALS] + config);
        auto title = object["title"].toString();
        title_map[QString().setNum(resulting_array.size())] = title;
        auto series = object["series"].toString();
        bool series_exist = false;
        for (auto key : series_map.keys()) {
            if (series_map.value(key) == series) {
                series_exist = true;
                break;
            }
        }
        if (!series_exist) {
            series_map[QString().setNum(resulting_array.size())] = series;
        }
        auto array = object["screens"].toArray();
        for (QJsonValueRef item : array) {
            auto record = item.toObject();
            record["index"] = (record["public"].toBool() ? resulting_array : hidden_array).size();
            (record["public"].toBool() ? resulting_array : hidden_array).push_back(record);
        }
    }
    QJsonObject result, hidden_result;
    result["records"] = resulting_array;
    result["reverse_index"] = reverse_index(resulting_array);
    result["title_map"] = title_map;
    result["series_map"] = series_map;
    hidden_result["records"] = hidden_array;
    hidden_result["reverse_index"] = reverse_index(hidden_array);
    QFile file(locations[PUBLIC_RECORDS]);
    QFile hidden_file(locations[HIDDEN_RECORDS]);
    save_json(hidden_result, hidden_file);
    auto message = save_json(result, file)
            ? "Обработано конфигов: " + QString().setNum(configs.size()) + ", "
              "скомпилировано публичных записей: " + QString().setNum(resulting_array.size()) + ", "
              "скрытых записей: " + QString().setNum(hidden_array.size())
            : "Не удалось сохранить файл.";
    ui->statusBar->showMessage(message);
}

void MainWindow::compile_journals_to_db() {
    Database::instance().init(Locations::instance()[DATABASE]);

    QDir dir = QDir(Locations::instance()[JOURNALS]);
    dir.setNameFilters(QStringList("*.json"));
    auto configs = dir.entryList(QDir::Files | QDir::NoDotAndDotDot);
    if (configs.contains(".test.json")) {
        configs.removeAt(configs.indexOf(".test.json"));
    }

    // Открытие или создание базы данных
    QFile db_file(Locations::instance()[DATABASE]);
    if (db_file.exists()) {
        // Если база данных уже существует, создаём резервную копию
        QFile::copy(Locations::instance()[DATABASE], Locations::instance()[DATABASE].chopped(3) + QString("_%1.db").arg(QDate::currentDate().toString(Qt::ISODate)));
    }

    // Подключение к базе данных
    Database& db = Database::instance();
    QSqlQuery query;

    db.reset_hashtag_table(query);
    QJsonObject hashtags_json = json_object(Locations::instance()[HASHTAGS]);
    db.add_hashtags_data(query, hashtags_json);

    // Очистка таблиц, если они существуют
    db.reset_main_tables(query);

    // Проход по всем json-конфигам
    for (int i = 0; i < configs.size(); ++i) {
        const auto& config = configs[i];
        QMetaObject::invokeMethod(progress_dialog, "update_progress", Q_ARG(int, i*100 / configs.size()), Q_ARG(const QString&, QString("Обработка файла: %1").arg(config)));
        auto object = json_object(Locations::instance()[JOURNALS] + config);
        db.add_journal_data(query, object);
    }
    QMetaObject::invokeMethod(progress_dialog, "update_progress", Q_ARG(int, 100), Q_ARG(const QString&, "База данных создана успешно."));
}

void MainWindow::export_text() {
    QFile file("exported_text.txt");
    if (!file.open(QIODevice::WriteOnly | QIODevice::Truncate)) {
        ui->statusBar->showMessage("Не удалось открыть выходной файл.");
        return;
    }
    QRegularExpression regex("(.*?)\\s[#&].*$");
    QTextStream out(&file);
    out.setCodec("UTF-8");
    QDir dir = QDir(locations[JOURNALS]);
    auto configs = dir.entryList(QDir::Files | QDir::NoDotAndDotDot);
    for (const auto& config : configs) {
//        if (config.startsWith("Kono")) break;
        auto object = json_object(locations[JOURNALS] + config);
        auto array = object["screens"].toArray();
        for (QJsonValueRef item : array) {
            auto record = item.toObject();
            if (record["public"].toBool()) {
                auto quote = record["caption"].toString();
//                auto i = regex.globalMatch(quote);
//                if (i.hasNext()) {
//                    auto match = i.next();
//                    quote = match.captured(1);
//                }
                out << quote + "\r\n";
            }
        }
    }
    file.close();
}

QJsonObject MainWindow::reverse_index(const QJsonArray& array) {
    QJsonObject result;
    for (int index = 0; index < array.size(); ++index) {
        auto id_array = array[index].toObject()["photo_ids"].toArray();
        for (QJsonValueRef id : id_array) {
            result[QString().setNum(id.toInt())] = index;
        }
    }
    return result;
}

