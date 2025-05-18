#include "include\journal_creation.h"

JournalCreation::JournalCreation(MainWindow* parent) : AbstractPreparationMode(parent)
{
    connect(manager, &VK_Manager::albums_ready, this, &JournalCreation::set_albums);
    connect(manager, &VK_Manager::photo_ids_ready, this, &JournalCreation::set_photo_ids);
    connect(parent, &MainWindow::key_press, this, &JournalCreation::keyPressEvent);
    connect(parent, &MainWindow::key_release, this, &JournalCreation::keyReleaseEvent);
    QDir dir = QDir(QFileDialog::getExistingDirectory(nullptr, "Открыть папку с кадрами",
                                                 locations[SCREENSHOTS]));
    pics = dir.entryList(QDir::Files | QDir::NoDotAndDotDot);
    title_map.insert(0, dir.dirName());
    QFile file(locations[QUOTES] + title_name() + ".txt");
    read_quote_file(file);
}

JournalCreation::~JournalCreation() {
    disconnect(parent, nullptr, this, nullptr);
    set_enabled(false);
}

void JournalCreation::start() {
    manager->get_albums();
}

void JournalCreation::keyPressEvent(QKeyEvent* event) {
    if (event->key() == Qt::Key_Control) {
        ui->back->setText("Отмена");
    }
}

void JournalCreation::keyReleaseEvent(QKeyEvent* event) {
    if (event->key() == Qt::Key_Control) {
        ui->back->setText("Назад");
    }
}

void JournalCreation::slider_change(int) {
    show_status();
}

void JournalCreation::skip_button() {
    pic_index = qMax(pic_index, pic_end_index) + 1;
    draw(pic_index);
    ui->slider->setValue(pic_index);
    show_status();
}

void JournalCreation::add_button() {
    // Adding one more image to current record
    pic_end_index = qMax(pic_index, pic_end_index) + 1;
    draw(pic_end_index);
    ui->slider->setValue(pic_end_index);
    show_status();
}

void JournalCreation::back_button() {
    if (ui->back->text() == "Назад") {
        // Going back in picture list
        if (pic_index == pic_end_index) {
            pic_end_index = 0;
        }
        int& current_index = pic_index > pic_end_index ? pic_index : pic_end_index;
        if (current_index == 0) return;
        draw(--current_index);
        ui->slider->setValue(current_index);
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
    show_status();
}

void JournalCreation::ok_button() {
    register_record();
    pic_index = qMax(pic_index, pic_end_index) + 1;
    pic_end_index = 0;
    ui->private_switch->setChecked(false);
    if (pic_index < pics.size()) {
        draw(pic_index);
        ui->slider->setValue(pic_index);
        show_text(++quote_index);
        show_status();
    } else {
        set_enabled(false);
        save_title_journal(title_name());
        update_quote_file(title_name());
    }
}

void JournalCreation::set_enabled(bool enable) {
    ui->main_view->setEnabled(enable);
    ui->back->setEnabled(enable && pic_index > 0);
    ui->ok->setEnabled(enable);
    ui->skip->setEnabled(enable);
    ui->add->setEnabled(enable && pics.size() > 1);
    ui->text->setEnabled(enable);
    ui->private_switch->setEnabled(enable);
    ui->slider->setValue(0);
}

void JournalCreation::draw(int index = 0) {
    if (!ui->offline->isChecked()) {
        manager->get_image(links[index]);
    } else {
        auto dir_path = locations[SCREENSHOTS] + title_name() + QDir::separator();
        auto image = QImage(dir_path + pics[index]);
        if (image.isNull()) {
            // Checking the reserve screenshot folder
            image = QImage(locations[SCREENSHOTS_NEW] + title_name() + QDir::separator() + pics[index]);
        }
        ui->image->setPixmap(scaled(image));
    }
    bool reached_end = index + 1 >= pics.size();
    ui->skip->setEnabled(!reached_end);
    ui->add->setEnabled(!reached_end);
    ui->back->setEnabled(index > 0);
}

void JournalCreation::show_text(int index) {
    if (quotes.size() <= index) return;
    ui->text->setText(quotes[index]);
}

void JournalCreation::show_status() {
    bool multiple_pics = pic_end_index > 0;
    QString s_quote = QString().setNum(quote_index + 1) + " из " + QString().setNum(quotes.size());
    QString s_pic = multiple_pics ? "кадры " : "кадр ";
    QString s_pic_index = QString().setNum(pic_index + 1) + (multiple_pics ? "-" + QString().setNum(pic_end_index + 1) : "");
    QString s_pic_from = " из " + QString().setNum(pics.size());
    ui->statusBar->showMessage("Цитата " + s_quote + ", " + s_pic + s_pic_index + s_pic_from);
}

bool JournalCreation::data_ready() {
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

void JournalCreation::set_albums(const QMap<QString, int>& ids) {
    album_ids = ids;
    if (album_ids.empty()) {
        ui->offline->setChecked(true);
        ui->statusBar->showMessage("Не удалось загрузить альбомы. Попробуйте авторизироваться вручную или продолжите работу оффлайн.");
    } else manager->get_photo_ids(album_ids[title_name()]);
}

void JournalCreation::set_photo_ids(const QVector<int>& ids, const QStringList& urls) {
    photo_ids = ids;
    links = urls;
    set_view(MAIN);
    if (data_ready()) {
        ui->ok->setText("Готово");
        ui->add->setText("Добавить");
        ui->skip->setText("Пропустить");
        ui->slider->setMaximum(pics.size() - 1);
        ui->stackedWidget->setCurrentIndex(0);
        draw(0);
        set_enabled(true);
        show_status();
    } else {
        set_enabled(false);
        ui->exit_mode->trigger();
    }
}

bool JournalCreation::read_quote_file(QFile& file) {
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
        return true;
    } else return false;
}

bool JournalCreation::update_quote_file(const QString& title) {
    QFile file(locations[QUOTES] + title + ".txt");
    if (!file.open(QIODevice::WriteOnly | QIODevice::Truncate)) {
        ui->statusBar->showMessage(QString("Не удалось открыть файл с цитатами: %1").arg(file.fileName()));
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

void JournalCreation::register_record() {
    Record record;
    for (int i = pic_index; i <= qMax(pic_index, pic_end_index); ++i) {
        record.pics.push_back(pics[i]);
        record.ids.push_back(photo_ids[i]);
        record.links.push_back(links[i]);
    }
    record.quote = ui->text->toPlainText();
    record.is_public = !ui->private_switch->isChecked();
    records.push_back(record);
}

void JournalCreation::save_title_journal(const QString& title) {
    QJsonArray record_array;
    for (const auto& record : records) {
        record_array.push_back(record.to_json());
    }
    QFile file(locations[JOURNALS] + title + ".json");
    QJsonObject object;
    object["title"] = title;
    object["title_caption"] = title;
    object["series"] = series_name(title);
    object["album_id"] = album_ids[title];
    object["screens"] = record_array;
    auto message = save_json(object, file)
            ? "Журнал скриншотов сохранён."
            : QString("Не удалось сохранить файл: %1").arg(file.fileName());
    ui->statusBar->showMessage(message);
}

QString JournalCreation::series_name(const QString& title) const {
    QRegularExpression regex("^(.*?)(?=\\s\\d|\\sOVA|$)");
    auto i = regex.globalMatch(title);
    if (i.hasNext()) {
        auto match = i.peekNext();
        return match.captured(1);
    } else return title;
}
