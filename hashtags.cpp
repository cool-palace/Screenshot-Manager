#include "mainwindow.h"
#include "ui_mainwindow.h"
#include <QDebug>

void MainWindow::get_hashtags() {
//    QFile file(locations[CONFIGS] + "hashtags.json");
//    if (!file.open(QIODevice::ReadOnly)) {
//        ui->statusBar->showMessage("Не удалось открыть файл с хэштегами.");
//    }
    for (int i = 0; i < 13; ++i) {
        ranked_hashtags.append(QStringList());
    }
    hashtags_json = json_object(locations[CONFIGS] + "result\\hashtags.json");

    for (const auto& key : hashtags_json.keys()) {
//        qDebug() << hashtags_json[key].toObject();
        full_hashtags_map[key] = Hashtag(key, hashtags_json[key].toObject());
//        if (full_hashtags.back().last_poll().toSecsSinceEpoch() > 0) {
//            poll_logs[key] = full_hashtags.back().last_poll().toSecsSinceEpoch();
//        }
        ranked_hashtags[hashtags_json[key].toObject()["rank"].toInt()].append(key);
        create_hashtag_button(key);
    }
//    QTextStream in(&file);
//    in.setCodec("UTF-8");
//    for (int i = 0; i < 10; ++i) {
//        ranked_hashtags.append(QStringList());
//    }
//    while (!in.atEnd()) {
//        auto line = in.readLine().split(' ');
//        int rank = line.size() > 1 ? line[1].toInt() : 0;
//        auto text = line.first();
//        if (rank >= ranked_hashtags.size()) {
//            for (int i = ranked_hashtags.size(); i <= rank; ++i) {
//                ranked_hashtags.append(QStringList());
//            }
//        }
//        ranked_hashtags[rank].append(text);
//        create_hashtag_button(text);
//    }
//    file.close();
    update_hashtag_grid();
}

void MainWindow::create_hashtag_button(const QString& text) {
    hashtags.insert(text, new HashtagButton(text));
    connect(hashtags[text], SIGNAL(filterEvent(const QChar&, const QString&, bool)), this, SLOT(filter_event(const QChar&, const QString&, bool)));
    connect(hashtags[text], SIGNAL(hashtagEvent(const QChar&, const QString&)), this, SLOT(hashtag_event(const QChar&, const QString&)));
    connect(hashtags[text], &HashtagButton::selected, this, &MainWindow::change_selected_hashtag);
}

void MainWindow::change_selected_hashtag(const QString& tag, HashtagPreview* preview) {
    if (selected_hashtags.contains(tag)) return;
    if (HashtagButton::current_preview_to_change()) HashtagButton::set_preview_to_change(nullptr);
    selected_hashtags.insert(tag, preview);
    selected_hashtags[tag]->set_hashtag(full_hashtags_map[tag]);
    QLayoutItem* child;
    while ((child = ui->preview_grid->takeAt(0))) {
        // Clearing items from the grid
    }
    for (auto item : selected_hashtags) {
        ui->preview_grid->addWidget(item);
    }
    set_view(PREVIEW);
}

void MainWindow::hashtag_event(const QChar& c, const QString& text) {
    if (current_mode == IDLE || current_mode == RELEASE_PREPARATION) return;
//    if (!filters.isEmpty()) {
//        filter_event(c, text, true);
//        return;
//    }
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

void MainWindow::update_hashtag_grid() {
    QLayoutItem* child;
    while ((child = ui->tag_grid->takeAt(0))) {
        // Clearing buttons from the grid
        child->widget()->hide();
    }
    int i = 0;
    if (ui->alphabet_order->isChecked()) {
        if (ui->hashtags_full->isChecked()) {
            // Displaying all buttons in alphabet order
            for (auto button : hashtags) {
                ui->tag_grid->addWidget(button, i / 13, i % 13);
                button->show();
                ++i;
            }
        } else if (ui->hashtags_newest->isChecked()) {
            // Sorting newest buttons to display them in alphabet order
            QMap<QString, HashtagButton*> tags;
            for (int index = 1; index < ranked_hashtags.size(); ++index) {
                for (const auto& text : ranked_hashtags[index]) {
                    tags.insert(text, hashtags[text]);
                }
            }
            for (auto button : tags) {
                ui->tag_grid->addWidget(button, i / 10, i % 10);
                button->show();
                ++i;
            }
        }
    } else if (ui->addition_order->isChecked()) {
        // Displaying buttons in addition order
        int columns_count = ui->hashtags_full->isChecked() ? 13 : 10;
        for (int index = ui->hashtags_full->isChecked() ? 0 : 1; index < ranked_hashtags.size(); ++index) {
            for (const auto& text : ranked_hashtags[index]) {
                ui->tag_grid->addWidget(hashtags[text], i / columns_count, i % columns_count);
                hashtags[text]->show();
                ++i;
            }
        }
    }
}

void MainWindow::load_hashtag_info() {
    for (auto button : hashtags) {
        button->reset();
    }
    HashtagButton::update_on_records(records.size());
    QSet<QString> hashtags_in_config;
    for (int index = 0; index < records.size(); ++index) {
        auto i = hashtag_match(records[index].quote);
        while (i.hasNext()) {
            auto hashtag = i.peekNext().captured(1);
            auto match = i.next().captured();
            hashtags_in_config.insert(hashtag);
            if (!hashtags.contains(hashtag)) {
                qDebug() << "Unexpected tag: " << hashtag;
                create_hashtag_button(hashtag);
                hashtags[hashtag]->highlight_unregistered();
                update_hashtag_grid();
            }
            hashtags[hashtag]->add_index(match.front(), index);
            hashtags_by_index[index].push_back(match);
        }
    }
    for (const auto& hashtag : hashtags_in_config) {
        hashtags[hashtag]->show_count();
    }
    highlight_current_hashtags(false);
    current_hashtags = hashtags_by_index[0];
    highlight_current_hashtags(true);
}

void MainWindow::recalculate_hashtags(bool increase) {
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

void MainWindow::update_hashtag_info() {
    recalculate_hashtags(false);
    update_current_hashtags();
    recalculate_hashtags(true);
}

void MainWindow::update_current_hashtags() {
    highlight_current_hashtags(false);
    current_hashtags.clear();
    auto i = hashtag_match(records[pic_index].quote);
    while (i.hasNext()) {
        current_hashtags.push_back(i.next().captured());
    }
    hashtags_by_index[pic_index] = current_hashtags;
    highlight_current_hashtags(true);
}

QRegularExpressionMatchIterator MainWindow::hashtag_match(const QString& text) const {
    QRegularExpression regex("[#&]([а-яё_123]+)");
    return regex.globalMatch(text);
}

void MainWindow::highlight_current_hashtags(bool enable) {
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

QString MainWindow::preprocessed(const QString& text) const {
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
    }{
        // Replacing #программирование with #айти
        QMap<QString, QString> map = {{" #программирование", " #айти"},
                                      {" #имя", " #имена"},
                                      {" #слово", " #слова"},
                                      {" #настроение", ""},
                                      {" #доброе_утро", " #приветствие"},
                                      {" #соцсети", " #интернет"},
                                      {" #чтение", " #книги"},
                                      {" #петух", " #геи"},
                                      {" #битва", " #поединок"},
                                      {" #смешное", ""},
                                      {" #серьёзное", ""},
                                      {" #комментарий", ""},
                                     };
        QRegularExpression it_regex("(.*)?( #программирование| #имя| #слово| "
                                    "#настроение| #доброе_утро| #соцсети| "
                                    "#чтение| #петух| #битва| #смешное| #серьёзное| #комментарий)(\\s.*)?$");
        auto i = it_regex.globalMatch(result);
        while (i.hasNext()) {
            auto match = i.next();
            result = match.captured(1) + map[match.captured(2)] + match.captured(3);
            qDebug() << result;
        }
    }{
        // Replacing #реакция+#hashtag with &hashtag
        QRegularExpression reaction_regex("(.*\\s)?#реакция(\\s.*)?$");
        auto i = reaction_regex.globalMatch(result);
        QSet<QString> exceptions({"#обман", "#глупость", "#игнор"});
        if (i.hasNext()) {
            auto match = i.peekNext();
            auto reactive_tags = match.captured(2).split(' ', Qt::SkipEmptyParts);
            for (auto& tag : reactive_tags) {
                if (tag[0] == '#' && !exceptions.contains(tag)) {
                    tag.replace(0, 1, "&");
                }
            }
            result = match.captured(1) + reactive_tags.join(" ");
            qDebug() << result;
        }
    }
    return result;
}

QSet<int> MainWindow::word_search(const QString& text) {
    QSet<int> result;
    for (int i = 0; i < records.size(); ++i) {
        QString quote;
        QRegularExpression regex("(.*?)?([#&])(.*)?$");
        auto it = regex.globalMatch(records[i].quote);
        if (it.hasNext()) {
            auto match = it.peekNext();
            quote = match.captured(1);
        } else quote = records[i].quote;
        if (quote.contains(text, Qt::CaseInsensitive)) {
            result.insert(i);
        }
    }
    return result;
}

QSet<int> MainWindow::records_by_public(bool publ) {
    QSet<int> result;
    for (int i = 0; i < records.size(); ++i) {
        if (records[i].is_public == publ) {
            result.insert(i);
        }
    }
    return result;
}

void MainWindow::filter_event(bool publ) {
    if (filters.contains("public") && filters["public"].include != publ) {
        filters.remove("public");
        apply_filters();
    }
    update_filters('p', "public", publ);
    if (filters.empty()) {
        exit_filtering();
        return;
    }
    // Disabling all buttons
    for (auto button : hashtags) {
        button->setDisabled(true);
    }
    // Handling the filter not used in the config
    if (filtration_results.isEmpty()) {
        ui->back->setDisabled(true);
        ui->ok->setDisabled(true);
    } else show_filtering_results();
    show_status();
}

void MainWindow::filter_event(const QString& text) {
    if (filters.contains(text) && filters.find(text).value().sign == 't') {
        return;
    }
    for (auto i = filters.begin(); i != filters.end(); i++) {
        if (i.value().sign == 't') {
            filters.erase(i);
            apply_filters();
            break;
        }
    }
    if (text.size() < 2) {
        exit_filtering();
        return;
    }
    update_filters('t', text, true);
    // Disabling all buttons
    for (auto button : hashtags) {
        button->setDisabled(true);
    }
    // Handling the filter not used in the config
    if (filtration_results.isEmpty()) {
        ui->back->setDisabled(true);
        ui->ok->setDisabled(true);
    } else show_filtering_results();
    show_status();
}

void MainWindow::filter_event(const QChar& sign, const QString& text, bool include) {
    if (filters.contains(text) && (sign != filters[text].sign || include != filters[text].include)) {
        QChar c = filters[text].sign;
        QString tip = c == '#' ? "левую кнопку" : c == '&' ? "правую кнопку" : "колесико";
        ui->statusBar->showMessage("Уже активен фильтр \"" + QString(c + text).simplified() + "\". "
                                   "Нажмите " + tip + " мыши, чтобы снять действующий фильтр.");
        return;
    }
    // Disabling all buttons
    for (auto button : hashtags) {
        button->setDisabled(true);
    }
    update_filters(sign, text, include);
    // Checking and unchecking filter buttons
    hashtags[text]->setChecked(filters.contains(text));
    // Enabling necessary buttons
    if (!filters.isEmpty()) {
        // Handling the filter not used in the config
        if (filtration_results.isEmpty()) {
            hashtags[text]->setEnabled(true);
            ui->back->setDisabled(true);
            ui->ok->setDisabled(true);
        } else show_filtering_results();
    } else exit_filtering();
    show_status();
}

void MainWindow::update_filters(const QChar& sign, const QString& text, bool include) {
    if (filters.isEmpty()) {
        ui->text->setDisabled(true);
        ui->slider->setDisabled(true);
        filters.insert(text, FilterSpecs(sign, include));
        if (!sign.isLetter()) hashtags[text]->highlight(include, true);
        apply_first_filter();
    } else if (!filters.contains(text)) {
        filters.insert(text, FilterSpecs(sign, include));
        if (!sign.isLetter()) {
            filter(hashtags[text]->indices(sign, include));
            hashtags[text]->highlight(include, true);
        } else if (sign == 't') {
            filter(word_search(text));
        } else {
            filter(records_by_public(include));
        }
    } else {
        if (!filters[text].sign.isLetter()) {
            hashtags[text]->highlight(include, false);
            hashtags[text]->setEnabled(true);
        }
        filters.remove(text);
        apply_filters();
    }
}

void MainWindow::apply_first_filter() {
    filtration_results.clear();
    auto i = filters.begin();
    if (!i.value().sign.isLetter()) {
        // First hashtag search
        for (int index : hashtags[i.key()]->indices(i.value().sign, i.value().include)) {
            filtration_results.insert(index, record_items[index]);
        }
    } else if (i.value().sign == 't') {
        // Full-text search
        for (int index : word_search(i.key())) {
            filtration_results.insert(index, record_items[index]);
        }
    } else {
        // Public filter
        for (int index : records_by_public(i.value().include)) {
            filtration_results.insert(index, record_items[index]);
        }
    }
}

void MainWindow::apply_filters() {
    for (auto i = filters.begin(); i != filters.end(); ++i) {
        if (i == filters.begin()) {
            apply_first_filter();
        } else {
            if (!i.value().sign.isLetter()) {
               filter(hashtags[i.key()]->indices(i.value().sign, i.value().include));
            } else if (i.value().sign == 't') {
                // Full-text search
                filter(word_search(i.key()));
            } else if (i.value().sign == 'p') {
                // Public filter
                filter(records_by_public(i.value().include));
            }
        }
    }
}

void MainWindow::show_filtering_results() {
    ui->slider->setValue(filtration_results.begin().key());
    ui->ok->setEnabled(filtration_results.size() > 1);
    ui->back->setDisabled(true);
    // Enabling buttons for possible non-zero result filters
    for (int index : filtration_results.keys()) {
        for (const auto& tag : hashtags_by_index[index]) {
            auto hashtag = tag.right(tag.size()-1);
            hashtags[hashtag]->setEnabled(true);
        }
    }
    // Making sure the excluding filter buttons stay enabled
    for (const auto& hashtag : filters.keys()) {
        if (!filters[hashtag].sign.isLetter()) {
            hashtags[hashtag]->highlight(filters[hashtag].include, true);
        }
    }
}

void MainWindow::exit_filtering() {
    filtration_results.clear();
    for (auto button : hashtags) {
        button->setEnabled(true);
    }
    ui->ok->setEnabled(pic_index < records.size() - 1);
    ui->back->setEnabled(pic_index > 0);
    ui->text->setEnabled(true);
    ui->slider->setEnabled(true);
}

void MainWindow::filter(const QSet<int>& second) {
    if (filtration_results.isEmpty()) return;
    QMap<int, RecordBase*> result;
    auto current_results = filtration_results.keys();
    auto keys = QSet<int>(current_results.begin(), current_results.end()).intersect(second);
    for (const auto& key : keys) {
        result.insert(key,record_items[key]);
    }
    filtration_results = result;
}

void MainWindow::update_hashtag_file() {
    QJsonObject object;
    for (const auto& tag : full_hashtags_map.keys()) {
        object[tag] = full_hashtags_map[tag].to_json();
    }
    QFile file(locations[CONFIGS] + "result\\hashtags.json");
    auto message = save_json(object, file)
            ? "Файл хэштегов сохранён."
            : "Не удалось сохранить файл.";
    ui->statusBar->showMessage(message);
}

void MainWindow::read_poll_logs() {
    auto log = json_object(locations[POLL_LOGS]);
    for (auto key : log.keys()) {
        poll_logs[key] = log.value(key).toInt();
    }
    HashtagPreview::poll_logs = &poll_logs;
}

void MainWindow::update_poll_logs() {
    QFile file(locations[POLL_LOGS]);
    QJsonObject object;
    for (auto key : poll_logs.keys()) {
        object[key] = poll_logs[key];
    }
    for (auto hashtag : selected_hashtags) {
        hashtag->update_log_info();
    }
    auto message = save_json(object, file)
            ? "Логи опросов обновлены."
            : "Не удалось обновить логи.";
    auto current_message = ui->statusBar->currentMessage();
    ui->statusBar->showMessage(current_message + ". " + message);
}
