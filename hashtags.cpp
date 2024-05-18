#include "mainwindow.h"
#include "ui_mainwindow.h"
#include <QDebug>

void MainWindow::get_hashtags() {
//    QFile file(locations[CONFIGS] + "hashtags.json");
//    if (!file.open(QIODevice::ReadOnly)) {
//        ui->statusBar->showMessage("Не удалось открыть файл с хэштегами.");
//    }
    for (int i = 0; i < 14; ++i) {
        ranked_hashtags.append(QStringList());
    }
    hashtags_json = json_object(locations[JOURNALS] + "result\\hashtags.json");

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
    connect(hashtags[text], SIGNAL(filterEvent(FilterType, const QString&)), this, SLOT(filter_event(FilterType, const QString&)));
    connect(hashtags[text], SIGNAL(hashtagEvent(const QChar&, const QString&)), this, SLOT(hashtag_event(const QChar&, const QString&)));
    connect(hashtags[text], &HashtagButton::selected, this, &MainWindow::change_selected_hashtag);
}

void MainWindow::change_selected_hashtag(const QString& tag, HashtagPreview* preview) {
    if (selected_hashtags.contains(tag)) return;
    if (HashtagButton::current_preview_to_change()) HashtagButton::set_preview_to_change(nullptr);
    selected_hashtags.insert(tag, preview);
    selected_hashtags[tag]->set_hashtag(full_hashtags_map[tag]);
    clear_grid(ui->preview_grid, false);
    for (auto item : selected_hashtags) {
        ui->preview_grid->addWidget(item);
    }
    set_view(PREVIEW);
}

void MainWindow::hashtag_event(const QChar& c, const QString& text) {
    if (current_mode == IDLE || current_mode == RELEASE_PREPARATION) return;
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
    clear_grid(ui->tag_grid);
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

QSet<int> MainWindow::checked_title_records() {
    QSet<int> result;
    for (auto title_item : title_items) {
        RecordTitleItem* item = dynamic_cast<RecordTitleItem*>(title_item);
        if (item->is_checked()) {
            result += item->indices();
        }
    }
    return result;
}

QSet<int> MainWindow::records_by_date(int days) {
    QSet<int> result;
    int limit = QDateTime::currentDateTime().addDays(-days).toSecsSinceEpoch();
    for (int i = 0; i < records.size(); ++i) {
        int id = records[i].ids[0];
        if (!logs.contains(id) || logs[id] < limit) {
            result.insert(i);
        }
    }
    return result;
}

void MainWindow::filter_event(bool publ) {
    FilterType type = publ ? FilterType::PUBLIC : FilterType::HIDDEN;
    // Public filters
    if (filters.contains("public") && filters["public"] != type) {
        filters.remove("public");
        apply_filters();
    }
    update_filters(type, "public");
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
    // Filters for text search
    if (filters.contains(text) && filters.find(text).value() == FilterType::TEXT) {
        // Re-entering same text does nothing
        return;
    }
    for (auto i = filters.begin(); i != filters.end(); i++) {
        if (i.value() == FilterType::TEXT) {
            filters.erase(i);
            apply_filters();
            break;
        }
    }
    if (text.size() < 2) {
        // Extremely short or empty queries are not searched
        if (filters.empty()) {
            exit_filtering();
        } else show_filtering_results();
        return;
    }
    update_filters(FilterType::TEXT, text);
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

void MainWindow::filter_event(FilterType type, const QString& text) {
    // Filters for hashtag
    if (filters.contains(text) && (type != filters[text])) {
        auto current_type = filters[text];
        QChar c = current_type & FilterType::ANY_TAG
                ? ' ' : current_type & FilterType::HASHTAG
                  ? '#' : '&';
        QString tip = current_type & FilterType::ANY_TAG
                ? "колесико" : current_type & FilterType::HASHTAG
                  ? "левую кнопку" : "правую кнопку";
        ui->statusBar->showMessage("Уже активен фильтр \"" + QString(c + text).simplified() + "\". "
                                   "Нажмите " + tip + " мыши, чтобы снять действующий фильтр.");
        return;
    }
    // Disabling all buttons
    for (auto button : hashtags) {
        button->setDisabled(true);
    }
    update_filters(type, text);
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

void MainWindow::filter_event(RecordTitleItem*, bool set_filter) {
    // Filters for titles
    ui->titles_set_filter->setEnabled(!set_filter);
    ui->titles_reset_filter->setEnabled(set_filter);
    update_filters(FilterType::TITLE, "title");
    if (filters.isEmpty()) {
        exit_filtering();
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

void MainWindow::filter_event(int days) {
    // Filters for date
    update_filters(FilterType::DATE, "date");
    if (filters.isEmpty()) {
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

void MainWindow::update_filters(FilterType type, const QString& text) {
    if (filters.isEmpty()) {
        // First filter handling
        ui->text->setDisabled(true);
        ui->slider->setDisabled(true);
        filters.insert(text, type);
        if (type & FilterType::ANY_TAG) hashtags[text]->highlight(type, true);
        apply_first_filter();
    } else if (!filters.contains(text)) {
        // Adding new filter
        filters.insert(text, type);
        if (type & FilterType::ANY_TAG) {
            // Handling hashtags
            filter(hashtags[text]->indices(type));
            hashtags[text]->highlight(type, true);
        } else if (type == FilterType::TEXT) {
            // Handling word search
            filter(word_search(text));
        } else if (type & FilterType::PUBLIC) {
            // Handling public filter
            filter(records_by_public(type == FilterType::PUBLIC));
        } else if (type == FilterType::TITLE) {
            // Handling title filter
            filter(checked_title_records());
        }
    } else {
        // Removing existing filter
        if (type & FilterType::ANY_TAG) {
            // Reset hashtag button
            hashtags[text]->highlight(type, false);
            hashtags[text]->setEnabled(true);
        }
        filters.remove(text);
        apply_filters();
    }
}

void MainWindow::apply_first_filter() {
    filtration_results.clear();
    auto i = filters.begin();
    auto type = i.value();
    if (type & FilterType::ANY_TAG) {
        // First hashtag search
        for (int index : hashtags[i.key()]->indices(type)) {
            filtration_results.insert(index, record_items[index]);
        }
    } else if (type == FilterType::TEXT) {
        // Full-text search
        for (int index : word_search(i.key())) {
            filtration_results.insert(index, record_items[index]);
        }
    } else if (type & FilterType::PUBLIC) {
        // Public filter
        for (int index : records_by_public(type == FilterType::PUBLIC)) {
            filtration_results.insert(index, record_items[index]);
        }
    } else if (type == FilterType::TITLE) {
        // Title filter
        for (int index : checked_title_records()) {
            filtration_results.insert(index, record_items[index]);
        }
    } else if (type == FilterType::DATE) {
        // Date filter
        for (int index : records_by_date(ui->last_used_days->value())) {
            filtration_results.insert(index, record_items[index]);
        }
    }
}

void MainWindow::apply_filters() {
    for (auto i = filters.begin(); i != filters.end(); ++i) {
        if (i == filters.begin()) {
            apply_first_filter();
        } else {
            auto type = i.value();
            if (type & FilterType::ANY_TAG) {
               filter(hashtags[i.key()]->indices(type));
            } else if (type == FilterType::TEXT) {
                // Full-text search
                filter(word_search(i.key()));
            } else if (type & FilterType::PUBLIC) {
                // Public filter
                filter(records_by_public(type == FilterType::PUBLIC));
            } else if (type == FilterType::TITLE) {
                // Title filter
                filter(checked_title_records());
            } else if (type == FilterType::DATE) {
                // Date filter
                filter(records_by_date(ui->last_used_days->value()));
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
            auto hashtag = tag.right(tag.size() - 1);
            hashtags[hashtag]->setEnabled(true);
        }
    }
    // Making sure the excluding filter buttons stay enabled
    for (const auto& hashtag : filters.keys()) {
        if (filters[hashtag] & FilterType::ANY_TAG) {
            hashtags[hashtag]->highlight(filters[hashtag], true);
        }
    }
    ui->statusBar->showMessage(QString("Действует фильтров: %1, найдено результатов: %2").arg(filters.size()).arg(filtration_results.size()));
}

void MainWindow::exit_filtering() {
    filtration_results.clear();
    for (auto button : hashtags) {
        button->setEnabled(true);
        button->setChecked(false);
    }
    ui->ok->setEnabled(pic_index < records.size() - 1);
    ui->back->setEnabled(pic_index > 0);
    ui->text->setEnabled(true);
    ui->slider->setEnabled(true);
    ui->statusBar->showMessage(QString("Фильтры сняты, всего записей: %1").arg(records.size()));
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
    QFile file(locations[JOURNALS] + "result\\hashtags.json");
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
