#include "release_preparation.h"

ReleasePreparation::ReleasePreparation(MainWindow* parent) : AbstractOperationMode(parent)
{
    connect(manager, &VK_Manager::posted_successfully, this, &ReleasePreparation::posting_success);
    connect(manager, &VK_Manager::post_failed, this, &ReleasePreparation::posting_fail);
    connect(manager, &VK_Manager::poll_ready, this, &ReleasePreparation::post_poll);
    connect(manager, &VK_Manager::poll_posted_successfully, this, &ReleasePreparation::poll_posting_success);
    connect(manager, &VK_Manager::poll_post_failed, this, &ReleasePreparation::poll_posting_fail);
    connect(ui->poll_preparation, &QAction::triggered, this, &ReleasePreparation::poll_preparation);
    connect(ui->generate, &QPushButton::clicked, this, &ReleasePreparation::generate_button);
    connect(ui->post, &QPushButton::clicked, this, &ReleasePreparation::post_button);
    connect(ui->last_used_limit, &QCheckBox::stateChanged, [this](int state) {
        bool checked = static_cast<bool>(state);
        ui->last_used_days->setEnabled(checked);
        filter_event(ui->last_used_days->value());
    });
    connect(ui->last_used_days, QOverload<int>::of(&QSpinBox::valueChanged), [this](int days) {
        filter_event(days);     // First call removes last date filter
        filter_event(days);     // Second call sets new date filter
    });
    connect(ui->check_log, &QAction::triggered, this, &ReleasePreparation::check_logs);
    connect(ui->selected_tags_analysis, &QAction::triggered, this, &ReleasePreparation::tag_pairing_analysis);

    connect(ui->size_limit, QOverload<int>::of(&QComboBox::currentIndexChanged), [this](int index) {
        // Removing existing size filter
        if (filters.contains("size")) {
            filter_event(filters["size"]);
        }
        switch (index) {
        case 1:
            filter_event(SINGLE);
            break;
        case 2:
            filter_event(MULTIPLE);
            break;
        default:
            break;
        }
    });
    connect(ui->series_limit, QOverload<int>::of(&QComboBox::currentIndexChanged), [this](int index) {
        switch (index) {
        case 0:
            emit ui->titles_reset_filter->clicked(true);
            break;
        case 1:
            for (auto title : log_shortlist_series) {
                dynamic_cast<RecordTitleItem*>(title_items_map.value(title))->set_checked(false);
            }
            emit ui->titles_set_filter->clicked(true);
            break;
        default:
            break;
        }
    });
}

ReleasePreparation::~ReleasePreparation() {
    clear_grid(ui->preview_grid);
    for (auto item : selected_hashtags) {
        delete item;
    }
    for (auto item : selected_records) {
        delete item;
    }
    set_enabled(false);
}

void ReleasePreparation::start() {
    get_hashtags();
    if (data_ready()) {
        load_hashtag_info();
        read_poll_logs();
        ui->date->setMinimumDate(QDate::currentDate());
        ui->date->setDate(QTime::currentTime() < QTime(3,0)
                              ? QDate::currentDate()
                              : QDate::currentDate().addDays(1));
    {
        bool weekend = ui->date->date().dayOfWeek() > 5;
        ui->time->setTime(weekend ? QTime(10,0) : QTime(8,0));
        ui->poll_end_time->setTime(QTime(21,0));
        if (weekend) ui->quantity->setValue(6);
    }
        RecordPreview::records = &records;
        ui->last_used_limit->setChecked(true);
        set_enabled(true);
        ui->generate->click();
        set_view(PREVIEW);
        return;
    } else set_enabled(false);

    //    ui->stackedWidget->setCurrentIndex(0);
}

void ReleasePreparation::keyPressEvent(QKeyEvent * event) {
    if (event->key() == Qt::Key_Control) {
        if (current_view != PREVIEW) {
            set_view(MAIN);
        }
    }
}

void ReleasePreparation::keyReleaseEvent(QKeyEvent * event) {
    switch (event->key()) {
    case Qt::Key_Control:
        if (current_view != PREVIEW) {
            set_view(LIST);
        }
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

void ReleasePreparation::set_view(View view) {
    if (view == current_view) return;
    current_view = view;
    switch (view) {
    case MAIN:
        ui->stacked_view->setCurrentIndex(0);
        ui->stackedWidget->setCurrentIndex(1);
        break;
    case LIST: case GALLERY:
        ui->stacked_view->setCurrentIndex(1);
        lay_previews(ui->page_index->value());
        break;
    case PREVIEW:
        ui->stacked_view->setCurrentIndex(2);
        break;
    case TITLES:
        ui->stacked_view->setCurrentIndex(3);
        lay_titles();
        break;
    }
    ui->main_view->setChecked(current_view == MAIN);
    ui->list_view->setChecked(current_view == LIST);
    ui->gallery_view->setChecked(current_view == GALLERY);
    ui->preview_view->setChecked(current_view == PREVIEW);
    ui->title_view->setChecked(current_view == TITLES);
}

void ReleasePreparation::set_enabled(bool enable) {
    ui->title_view->setEnabled(enable);
    ui->list_view->setEnabled(enable);
    ui->gallery_view->setEnabled(enable);
    ui->page_index->setEnabled(enable);
    ui->pics_per_page->setEnabled(enable);
    ui->search_bar->setEnabled(enable);
    ui->word_search_button->setEnabled(enable);
    ui->word_search_reset->setEnabled(enable);
    ui->titles_check_all->setEnabled(enable);
    ui->titles_reset_filter->setEnabled(enable);
    ui->titles_set_filter->setEnabled(enable);
    ui->titles_uncheck_all->setEnabled(enable);
    ui->alphabet_order->setEnabled(enable);
    ui->addition_order->setEnabled(enable);
    ui->hashtags_full->setEnabled(enable);
    ui->hashtags_newest->setEnabled(enable);
    ui->check_log->setEnabled(enable);
    ui->preview_view->setEnabled(enable);
    ui->date->setEnabled(enable);
    ui->generate->setEnabled(enable);
    ui->interval->setEnabled(enable);
    ui->last_used_days->setEnabled(enable);
    ui->last_used_limit->setEnabled(enable);
    ui->quantity->setEnabled(enable);
    ui->size_limit->setEnabled(enable);
    ui->series_limit->setEnabled(enable);
    ui->time->setEnabled(enable);
    ui->post->setEnabled(enable);
    ui->poll_preparation->setEnabled(enable);
    ui->poll_end_time->setEnabled(enable);
    if (enable) {
        ui->back->hide();
        ui->text->hide();
        ui->ok->hide();
    } else {
        ui->back->show();
        ui->text->show();
        ui->ok->show();
        ui->ok->setDisabled(!enable);
    }
}

bool ReleasePreparation::open_public_journal() {
    auto json_file = json_object(locations[JOURNALS] + "result\\public_records.json");
    if (!json_file.contains("records")) {
        ui->statusBar->showMessage("Неверный формат файла public_records.json" );
        return false;
    }
    auto records_array = json_file.value("records").toArray();
    for (QJsonValueRef r : records_array) {
        Record record;
        auto object = r.toObject();
        record.quote = object["caption"].toString(); // -preprocessed
        record.is_public = object["public"].toBool();
        auto filename_array = object["filenames"].toArray();
        for (QJsonValueRef name : filename_array) {
            record.pics.push_back(name.toString());
        }
        auto id_array = object["photo_ids"].toArray();
        for (QJsonValueRef id : id_array) {
            record.ids.push_back(id.toInt());
        }
        auto link_array = object["links"].toArray();
        for (QJsonValueRef link : link_array) {
            record.links.push_back(link.toString());
        }
        records.push_back(record);
    }
    read_logs();
    auto reverse = json_file.value("reverse_index").toObject();
    for (auto index : reverse.keys()) {
        records_by_photo_ids[index.toInt()] = reverse[index].toInt();
    }
    auto titles = json_file.value("title_map").toObject();
    for (auto index : titles.keys()) {
        title_map[index.toInt()] = titles[index].toString();
    }
    auto series = json_file.value("series_map").toObject();
    for (auto index : series.keys()) {
        series_map[index.toInt()] = series[index].toString();
    }
    // Creating title items for series
    for (auto index : series_map.keys()) {
        int size = series_range(index).second - index + 1;
        title_items.push_back(new RecordTitleItem(series_map.value(index), path(index) + records[index].pics[0], size, index));
        title_items_map.insert(series_map.value(index), title_items.back());
        ui->title_grid->addWidget(title_items.back());
    }
    // Creating record items
    for (int i = records.size() - records_array.size(); i < records.size(); ++i) {
        record_items.push_back(new RecordItem(records[i], i, path(i)));
        if (logs.contains(records[i].ids[0])) {
            dynamic_cast<RecordItem*>(record_items.back())->include_log_info(logs.value(records[i].ids[0]));
        }
        ui->view_grid->addWidget(record_items.back());
        connect(record_items[i], &RecordItem::selected, [this](int index){
            selected_records[pic_index]->set_index(index);
            set_view(PREVIEW);
        });
    }
    return !records.empty();
}

void ReleasePreparation::lay_previews(int page) {
    if (current_view == MAIN) return;
    int pics_per_page = ui->pics_per_page->value();
    int total_previews = filtration_results.isEmpty()
                          ? records.size()
                          : filtration_results.values().size();
    ui->page_index->setMaximum(total_previews / pics_per_page + 1);
    clear_grid(ui->view_grid);
    if (!ui->check_log->isChecked()) {
        const auto& items = filtration_results.empty()
                            ? record_items
                            : filtration_results.values();
        for (int i = (page - 1) * pics_per_page ; i < qMin(items.size(), page * pics_per_page); ++i) {
            QtConcurrent::run(items[i], &RecordBase::load_thumbmnail);
            if (current_view == LIST) {
                items[i]->set_list_view();
                ui->view_grid->addWidget(items[i], i, 0);
            } else {
                items[i]->set_gallery_view();
                ui->view_grid->addWidget(items[i], i/10, i%10);
            }
        }
    } else {
        // Log checking
        auto keys = filtration_results.keys();
        int total_items = keys.size();
        int start_index = total_items - (page - 1) * pics_per_page - 1;
        int end_index = qMax(total_items - page * pics_per_page, 0);
        for (int i = start_index; i >= end_index && i >= 0; --i) {
            auto item = filtration_results[keys[i]];
            QtConcurrent::run(item, &RecordBase::load_thumbmnail);
            if (current_view == LIST) {
                item->set_list_view();
                ui->view_grid->addWidget(item, (start_index - i), 0);
            } else {
                item->set_gallery_view();
                ui->view_grid->addWidget(item, i/10, i%10);
            }
        }
    }
}

void ReleasePreparation::create_hashtag_button(const QString & text) {
    hashtags.insert(text, new HashtagButton(text));
    connect(hashtags[text], SIGNAL(filterEvent(FilterType, const QString&)), this, SLOT(filter_event(FilterType, const QString&)));
    connect(hashtags[text], &HashtagButton::selected, this, &ReleasePreparation::change_selected_hashtag);
}

bool ReleasePreparation::data_ready() {
    return open_public_journal();
}

void ReleasePreparation::check_logs() {
    filter_event(logs);
    lay_previews();
    set_view(LIST);
}

void ReleasePreparation::generate_button() {
    clear_grid(ui->preview_grid);
    ui->poll_preparation->isChecked() ? generate_poll() : generate_release();
}

void ReleasePreparation::generate_release() {
    for (auto record : selected_records) {
        delete record;
    }
    selected_records.clear();
    find_recently_posted_series();
    QDateTime time = QDateTime(ui->date->date(), ui->time->time(), Qt::LocalTime);
    for (int i = 0; i < ui->quantity->value(); ++i) {
        int r_index = random_index();
        selected_records.push_back(new RecordPreview(records[r_index], r_index, time));
        connect(selected_records.back(), &RecordPreview::search_start, [this](int index){
            pic_index = index;
            set_view(LIST);
        });
        connect(selected_records.back(), &RecordPreview::reroll_request, [this](RecordPreview* preview){
            preview->set_index(random_index());
        });
        time = time.addSecs(ui->interval->time().hour()*3600 + ui->interval->time().minute()*60);
        ui->preview_grid->addWidget(selected_records.back());
        selected_records.back()->set_list_view();
    }
    RecordPreview::selected_records = &selected_records;
}

void ReleasePreparation::generate_poll() {
    for (auto tag : selected_hashtags) {
        delete tag;
    }
    selected_hashtags.clear();
    while (selected_hashtags.size() < ui->quantity->value()) {
        int r_index = QRandomGenerator::global()->bounded(full_hashtags_map.keys().size());
        auto tag = full_hashtags_map.keys()[r_index];
        if (!selected_hashtags.contains(tag)) {
            selected_hashtags[tag] = new HashtagPreview();
            create_hashtag_preview_connections(tag);
            // Setting the actual tag contents
            selected_hashtags[tag]->set_hashtag(full_hashtags_map[tag]);
        }
    }
    tag_pairing_analysis();
    for (const auto& tag : selected_hashtags) {
        ui->preview_grid->addWidget(tag);
    }
}

void ReleasePreparation::post_button() {
    if (!ui->poll_preparation->isChecked()) {
        post_counter = selected_records.size();
        for (auto record : selected_records) {
            int index = record->get_index();
            manager->post(index, attachments(index), record->timestamp());
        }
    } else {
        manager->get_poll(options(), ui->poll_end_time->dateTime().toSecsSinceEpoch());
    }
}

void ReleasePreparation::posting_success(int index, int date) {
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
}

void ReleasePreparation::posting_fail(int index, const QString& reply) {
    if (post_counter < selected_records.size()) {
        update_logs();
        post_counter = 0;
    }
    ui->statusBar->showMessage(QString("Не удалось опубликовать запись %1").arg(index+1));
    QMessageBox msgBox(QMessageBox::Critical, "Ошибка", reply);
    msgBox.exec();
}

void ReleasePreparation::post_poll(int id) {
    int time = QDateTime(ui->date->date(), ui->time->time(), Qt::LocalTime).toSecsSinceEpoch();
    manager->post(poll_message(), id, time);
}

void ReleasePreparation::poll_posting_success() {
    int time = QDateTime(ui->date->date(), ui->time->time(), Qt::LocalTime).toSecsSinceEpoch();
    for (const auto& tag : selected_hashtags.keys()) {
        poll_logs[tag] = time;
        if (selected_hashtags[tag]->is_edited()) {
            qDebug() << selected_hashtags[tag]->text_description();
            full_hashtags_map[tag].set_description(selected_hashtags[tag]->text_description());
        }
    }
    update_hashtag_file();
    update_poll_logs();
    auto current_message = ui->statusBar->currentMessage();
    ui->statusBar->showMessage(current_message + ". Опрос опубликован");
}

void ReleasePreparation::poll_posting_fail(const QString& reply) {
    ui->statusBar->showMessage(QString("Не удалось опубликовать опрос"));
    QMessageBox msgBox(QMessageBox::Critical, "Ошибка", reply);
    msgBox.exec();
}

void ReleasePreparation::poll_preparation(bool poll_mode) {
    clear_grid(ui->preview_grid);
    if (poll_mode) for (auto tag : selected_hashtags) {
        ui->preview_grid->addWidget(tag);
        tag->show();
    } else for (auto record : selected_records) {
        ui->preview_grid->addWidget(record);
        record->show();
    }
    ui->last_used_limit->setEnabled(!poll_mode);
    ui->last_used_days->setEnabled(!poll_mode);
    ui->size_limit->setEnabled(!poll_mode);
    ui->series_limit->setEnabled(!poll_mode);
    ui->time->setTime(poll_mode ? QTime(12,5) : QTime(8,0));
    ui->quantity->setValue(poll_mode ? 6 : 7);
    ui->label_interval->setText(poll_mode ? "Конец опроса" : "Интервал");
    ui->stackedWidget_interval->setCurrentIndex(poll_mode ? 1 : 0);
    int day = ui->date->date().dayOfWeek();
    ui->poll_end_time->setDate(ui->date->date().addDays(4 - day)); // Set to end on thursday evening
}

void ReleasePreparation::tag_pairing_analysis() {
    selected_tag_pairings.clear();
    auto selected_tags = selected_hashtags.keys();
    for (auto it = selected_hashtags.begin(); it != selected_hashtags.end(); ++it) {
        QList<int> temp;
        // Copying already checked values
        for (int i = 0; i < selected_tag_pairings.size(); ++i) {
            temp.append(selected_tag_pairings[i][selected_tag_pairings.size()]);
        }
        auto tag = it.key();
        // Checking the diagonal value
        remove_hashtag_filters();
        emit hashtags[tag]->filterEvent(FilterType::ANY_TAG, tag);
        temp.append(hashtags[tag]->current_count());
        // Counting new values
        for (int i = selected_tag_pairings.size() + 1; i < selected_tags.size(); ++i) {
            auto current_tag = selected_tags[i];
            if (hashtags[current_tag]->isEnabled()) {
                temp.append(hashtags[current_tag]->current_count());
            } else temp.append(0);
        }
        selected_tag_pairings.append(temp);
    }
    remove_hashtag_filters();
    for (int i = 0; i < selected_tag_pairings.size(); ++i) {
        qDebug() << selected_tags[i] << selected_tag_pairings[i];
    }
}

void ReleasePreparation::read_poll_logs() {
    auto log = json_object(locations[POLL_LOGS]);
    for (auto key : log.keys()) {
        poll_logs[key] = log.value(key).toInt();
    }
    HashtagPreview::poll_logs = &poll_logs;
}

void ReleasePreparation::update_poll_logs() {
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

void ReleasePreparation::update_hashtag_file() {
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

void ReleasePreparation::change_selected_hashtag(const QString& tag, HashtagPreview* preview) {
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

void ReleasePreparation::create_hashtag_preview_connections(const QString& tag) {
    connect(selected_hashtags[tag], &HashtagPreview::reroll_request, [this](const QString& old_tag){
        // Saving the pointer to the hashtag preview to replace
        auto preview = selected_hashtags[old_tag];
        selected_hashtags.remove(old_tag);
        QString tag;
        do {
            int index = QRandomGenerator::global()->bounded(full_hashtags_map.keys().size());
            tag = full_hashtags_map.keys()[index];
            qDebug() << tag << selected_hashtags.contains(tag);
        } while (selected_hashtags.contains(tag));
        change_selected_hashtag(tag, preview);
        tag_pairing_analysis();
    });
    connect(selected_hashtags[tag], &HashtagPreview::search_start, [this](const QString& old_tag){
        // Saving the pointer to the hashtag preview to replace
        auto preview = selected_hashtags[old_tag];
        selected_hashtags.remove(old_tag);
        HashtagButton::set_preview_to_change(preview);
        set_view(MAIN);
    });
    connect(selected_hashtags[tag], &HashtagPreview::count_request, [this](const QString& tag){
        // Saving the pointer to the hashtag preview to replace
        int count = hashtags[tag]->get_count();
        selected_hashtags[tag]->update_count(count);
    });
    connect(selected_hashtags[tag], &HashtagPreview::check_request, [this](const QString& tag){
        remove_hashtag_filters();
        emit hashtags[tag]->filterEvent(FilterType::ANY_TAG, tag);
        set_view(LIST);
    });
}

void ReleasePreparation::remove_hashtag_filters() {
    for (auto filter : filters.keys()) {
        // Cancelling any other hashtag filters
        if (filters[filter] & FilterType::ANY_TAG) {
            emit hashtags[filter]->filterEvent(FilterType::ANY_TAG, filter);
        }
    }
}

int ReleasePreparation::random_index() const {
    if (filtration_results.empty()) {
        // Getting random index from records
        return QRandomGenerator::global()->bounded(records.size());
    }
    // Getting random index from filtered records only
    auto filtered_indices = filtration_results.keys();
    int random_pre_index = QRandomGenerator::global()->bounded(filtered_indices.size());
    qDebug() << filtered_indices.size() << random_pre_index << filtered_indices[random_pre_index];
    return filtered_indices[random_pre_index];
}

QString ReleasePreparation::attachments(int index) const {
    QString result;
    for (const auto& photo_id : records[index].ids) {
        if (!result.isEmpty()) result += ",";
        result += manager->prefix() + QString().setNum(photo_id);
    }
    return result;
}

QString ReleasePreparation::options() const {
    QString result('[');
    for (const auto& tag : selected_hashtags) {
        if (result.size() > 1) result += ",";
        result += tag->option();
    }
    result += ']';
    return result;
}

QString ReleasePreparation::poll_message() const {
    QString text;
    int i = 1;
    for (auto it = selected_hashtags.cbegin(); it != selected_hashtags.cend(); ++it, ++i) {
        text.append(QString("%1. ").arg(i));
        text.append(it.value()->line() + it.value()->text_description() + '\n');
    }
    return text;
}

void ReleasePreparation::read_logs() {
    auto log = json_object(locations[LOGS_FILE]);
    for (auto key : log.keys()) {
        logs[key.toInt()] = log.value(key).toInt();
    }
    RecordPreview::logs = &logs;
}

void ReleasePreparation::update_logs() {
    QFile file(locations[LOGS_FILE]);
    QJsonObject object;
    for (auto key : logs.keys()) {
        object[QString().setNum(key)] = logs[key];
    }
    for (auto record : selected_records) {
        record->update_log_info(records[record->get_index()].ids.first());
    }
    auto message = save_json(object, file)
            ? "Логи публикаций обновлены."
            : "Не удалось обновить логи.";
    auto current_message = ui->statusBar->currentMessage();
    ui->statusBar->showMessage(current_message + ". " + message);
}

QPair<int, int> ReleasePreparation::series_range(int index) {
    // Returns starting and ending indices for a series containing records[index]
    int start, end;
    if (series_map.contains(index)) {
        auto it = series_map.find(index);
        if (++it != series_map.end()) {
            end = it.key() - 1;
        } else end = records.size() - 1;
        return qMakePair(index, end);
    }
    series_map[index] = QString();
    auto it = series_map.find(index);
    start = (--it).key();
    series_map.remove(index);
    if (++it != series_map.end()) {
        end = it.key() - 1;
    } else end = records.size() - 1;
    return qMakePair(start, end);
}

void ReleasePreparation::find_recently_posted_series() {
    // There is an option to check back all the title items and clear log_shortlist_series here first,
    // but in practice it seems more useful not to do it for now
    QDateTime time = QDateTime(ui->date->date(), ui->time->time(), Qt::LocalTime);
    for (auto id : logs.keys()) {
        int value = logs.value(id);
        int diff = QDateTime::fromSecsSinceEpoch(value, Qt::LocalTime).daysTo(time);
        if (diff == 1 || diff == -1) {
            // Saving records posted yesterday to shortlist
            log_shortlist_series.insert(series_name(records_by_photo_ids[id]));
        }
    }
    qDebug() << log_shortlist_series;
    if (ui->series_limit->currentIndex() == 1) {
        emit ui->series_limit->currentIndexChanged(1);
    }
}