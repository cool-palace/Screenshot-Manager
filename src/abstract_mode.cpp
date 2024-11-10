#include "include\abstract_mode.h"

AbstractMode::AbstractMode(MainWindow* parent)
    : parent(parent), ui(parent->get_ui()), manager(parent->get_vk()), locations(parent->get_locations())
{
    connect(manager, &VK_Manager::image_ready, this, &AbstractMode::set_loaded_image);
    connect(ui->slider, &QAbstractSlider::valueChanged, this, &AbstractMode::slider_change);
    connect(ui->page_index, QOverload<int>::of(&QSpinBox::valueChanged), this, &AbstractMode::lay_previews);
    connect(ui->main_view, &QAction::triggered, [this]() { set_view(MAIN); });
    connect(ui->list_view, &QAction::triggered, [this]() { set_view(LIST); });
    connect(ui->gallery_view, &QAction::triggered, [this]() { set_view(GALLERY); });
    connect(ui->preview_view, &QAction::triggered, [this]() { set_view(PREVIEW); });
    connect(ui->title_view, &QAction::triggered, [this]() { set_view(TITLES); });
    connect(ui->skip, &QPushButton::clicked, this, &AbstractMode::skip_button);
    connect(ui->add, &QPushButton::clicked, this, &AbstractMode::add_button);
    connect(ui->back, &QPushButton::clicked, this, &AbstractMode::back_button);
    connect(ui->ok, &QPushButton::clicked, this, &AbstractMode::ok_button);
//    ui->image->setScaledContents(true);
    ui->exit_mode->setEnabled(true);
    ui->main_view->setIcon(QIcon(":/images/icons8-full-image-80.png"));
    ui->toolBar->show();
}

AbstractMode::~AbstractMode() {
    clear_grid(ui->view_grid);
    for (auto item : record_items) {
        delete item;
    }
    disconnect(manager, nullptr, this, nullptr);
    disconnect(ui->slider, nullptr, this, nullptr);
    disconnect(ui->page_index, nullptr, this, nullptr);
    disconnect(ui->main_view, nullptr, this, nullptr);
    disconnect(ui->list_view, nullptr, this, nullptr);
    disconnect(ui->gallery_view, nullptr, this, nullptr);
    disconnect(ui->preview_view, nullptr, this, nullptr);
    disconnect(ui->title_view, nullptr, this, nullptr);
    disconnect(ui->skip, nullptr, this, nullptr);
    disconnect(ui->add, nullptr, this, nullptr);
    disconnect(ui->back, nullptr, this, nullptr);
    disconnect(ui->ok, nullptr, this, nullptr);
}

QPixmap AbstractMode::scaled(const QImage& source) const {
    return QPixmap::fromImage(source.scaled(ui->image->geometry().size(), Qt::KeepAspectRatio));
}

void AbstractMode::set_loaded_image(const QImage& image) {
    ui->image->setPixmap(scaled(image));
}

void AbstractMode::clear_grid(QLayout* layout, bool hide) {
    QLayoutItem* child;
    while ((child = layout->takeAt(0))) {
        // Clearing items from the grid
        if (hide) child->widget()->hide();
    }
}

QString AbstractMode::title_name(int index) {
    if (title_map.empty()) return QString();
    if (title_map.contains(index)) return title_map.value(index);
    title_map[index] = QString();
    auto it = title_map.find(index);
    auto title = (--it).value();
    title_map.remove(index);
    return title;
}

AbstractPreparationMode::AbstractPreparationMode(MainWindow *parent) : AbstractMode(parent) { }

void AbstractPreparationMode::set_view(View view) {
    if (view == current_view) return;
    current_view = view;
    switch (view) {
    case MAIN:
        ui->stacked_view->setCurrentIndex(1);
        ui->stackedWidget->setCurrentIndex(0);
        break;
    case LIST:
        ui->stacked_view->setCurrentIndex(2);
        lay_previews(ui->page_index->value());
        break;
    default:
        break;
    }
    ui->main_view->setChecked(current_view == MAIN);
    ui->list_view->setChecked(current_view == LIST);
}

bool AbstractPreparationMode::update_quote_file(const QString& title) {
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

AbstractOperationMode::AbstractOperationMode(MainWindow *parent) : AbstractMode(parent) {
    connect(ui->search_bar, &QLineEdit::returnPressed, [this]() {
        filter_event(ui->search_bar->text());
        lay_previews();
    });
    connect(ui->word_search_button, &QPushButton::clicked, [this]() { emit ui->search_bar->returnPressed(); });
    connect(ui->word_search_reset, &QPushButton::clicked, [this]() {
        ui->search_bar->clear();
        filter_event(ui->search_bar->text());
        lay_previews();
    });
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
    connect(ui->hashtag_order, QOverload<int>::of(&QComboBox::currentIndexChanged), [this]() {
        update_hashtag_grid();
    });
    connect(ui->hashtag_rank_min, QOverload<int>::of(&QSpinBox::valueChanged), [this](int value) {
        ui->hashtag_rank_max->setMinimum(value);
        update_hashtag_grid();
    });
    connect(ui->hashtag_rank_max, QOverload<int>::of(&QSpinBox::valueChanged), [this](int value) {
        ui->hashtag_rank_min->setMaximum(value);
        update_hashtag_grid();
    });
    connect(ui->titles_order, QOverload<int>::of(&QComboBox::currentIndexChanged), [this]() {
        lay_titles();
    });
    connect(ui->titles_check_all, &QPushButton::clicked, [this]() { check_titles(true); });
    connect(ui->titles_uncheck_all, &QPushButton::clicked, [this]() { check_titles(false); });
    connect(ui->titles_set_filter, &QPushButton::clicked, [this]() {
        filter_event(nullptr, true);  // Check type
    });
    connect(ui->titles_reset_filter, &QPushButton::clicked, [this]() {
//        if (ui->series_limit->isChecked()) ui->series_limit->setChecked(false);
        filter_event(nullptr, false); // Check type
    });
}

AbstractOperationMode::~AbstractOperationMode() {
    clear_grid(ui->tag_grid);
    for (auto item : hashtags) {
        delete item;
    }
    clear_grid(ui->title_grid);
    for (auto item : title_items) {
        delete item;
    }
    disconnect(ui->search_bar, nullptr, this, nullptr);
    disconnect(ui->word_search_button, nullptr, this, nullptr);
    disconnect(ui->word_search_reset, nullptr, this, nullptr);
    disconnect(ui->alphabet_order, nullptr, this, nullptr);
    disconnect(ui->addition_order, nullptr, this, nullptr);
    disconnect(ui->hashtags_full, nullptr, this, nullptr);
    disconnect(ui->hashtags_newest, nullptr, this, nullptr);
    disconnect(ui->titles_check_all, nullptr, this, nullptr);
    disconnect(ui->titles_uncheck_all, nullptr, this, nullptr);
    disconnect(ui->titles_set_filter, nullptr, this, nullptr);
    disconnect(ui->titles_reset_filter, nullptr, this, nullptr);
}

QRegularExpressionMatchIterator AbstractOperationMode::hashtag_match(const QString& text) const {
    QRegularExpression regex("[#&]([а-яё_123]+)");
    return regex.globalMatch(text);
}

void AbstractOperationMode::get_hashtags() {
    int max_rank = 0;
    int min_rank = 0;
    QJsonObject hashtags_json = json_object(locations[HASHTAGS]);
    for (const auto& key : hashtags_json.keys()) {
        auto object = hashtags_json[key].toObject();
        int rank = object["rank"].toInt();
        if (rank > max_rank) {
            max_rank = rank;
        } else if (rank < min_rank) min_rank = rank;
        while (rank >= ranked_hashtags.size()) {
            ranked_hashtags.append(QStringList());
        }
        full_hashtags_map[key] = Hashtag(key, object);
        ranked_hashtags[rank].append(key);
        create_hashtag_button(key);
    }
    ui->hashtag_rank_min->setMaximum(max_rank);
    ui->hashtag_rank_max->setMaximum(max_rank);
    ui->hashtag_rank_max->setValue(max_rank);
    update_hashtag_grid();
}

void AbstractOperationMode::update_hashtag_grid() {
    int button_width = 130;
    int columns = std::max(1, (ui->tag_scroll_area->width() - 100) / button_width);
    clear_grid(ui->tag_grid);
    using Iterator = QMap<QString, Hashtag>::const_iterator;
    QVector<Iterator> iters;
    iters.reserve(full_hashtags_map.size());
    for (auto it = full_hashtags_map.cbegin(); it != full_hashtags_map.cend(); ++it) {
        int rank = it.value().rank_index();
        if (rank >= ui->hashtag_rank_min->value() && rank <= ui->hashtag_rank_max->value()) {
            iters.push_back(it);
        }
    }
    static auto ranked_sort = [](const Iterator& it1, const Iterator& it2){
        if (it1->rank_index() != it2->rank_index()) {
            return it1->rank_index() < it2->rank_index();
        }
        return it1.key() < it2.key();
    };
    static auto popular_sort = [this](const Iterator& it1, const Iterator& it2){
        return hashtags.value(it1.key())->current_count() > hashtags.value(it2.key())->current_count();
    };
//    static auto alphabet_sort = [](const Iterator& it1, const Iterator& it2){
//        return it1->text() < it2->text();
//    };
    if (ui->hashtag_order->currentIndex() == 1) {
        std::sort(iters.begin(), iters.end(), ranked_sort);
    } else if (ui->hashtag_order->currentIndex() == 2) {
        std::sort(iters.begin(), iters.end(), popular_sort);
    }
    int i = 0;
    for (const auto iter : iters) {
        const auto button = hashtags.value(iter.key());
        ui->tag_grid->addWidget(button, i / columns, i % columns);
        button->show();
        ++i;
    }
}

void AbstractOperationMode::load_hashtag_info() {
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
}

void AbstractOperationMode::filter_event(bool publ) {
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

void AbstractOperationMode::filter_event(const QString& text) {
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

void AbstractOperationMode::filter_event(FilterType type, const QString& text) {
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

void AbstractOperationMode::filter_event(RecordTitleItem*, bool set_filter) {
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

void AbstractOperationMode::filter_event(int days) {
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

void AbstractOperationMode::filter_event(const QMap<int, int>&) {
    // Filter for log watching
    update_filters(FilterType::LOGS, "logs");
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

void AbstractOperationMode::filter_event(FilterType type) {
    // Filter for record size
    update_filters(type, "size");
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

void AbstractOperationMode::set_view(View view) {
    if (view == current_view) return;
    current_view = view;
    switch (view) {
    case MAIN:
        ui->stacked_view->setCurrentIndex(1);
        ui->stackedWidget->setCurrentIndex(0);
        break;
    case LIST: case GALLERY:
        ui->stacked_view->setCurrentIndex(2);
        lay_previews(ui->page_index->value());
        break;
    case PREVIEW:
        ui->stacked_view->setCurrentIndex(3);
        break;
    case TITLES:
        ui->stacked_view->setCurrentIndex(4);
        lay_titles();
        break;
    }
    ui->main_view->setChecked(current_view == MAIN);
    ui->list_view->setChecked(current_view == LIST);
    ui->gallery_view->setChecked(current_view == GALLERY);
    ui->preview_view->setChecked(current_view == PREVIEW);
    ui->title_view->setChecked(current_view == TITLES);
}

void AbstractOperationMode::lay_titles() {
    int title_width = 192;
    int columns = std::max(1, (ui->titles_scroll_area->width() - 100) / title_width);
    clear_grid(ui->title_grid);
    using Iterator = QMap<QString, RecordBase*>::const_iterator;
    QVector<Iterator> iters;
    iters.reserve(title_items.size());
    for (auto it = title_items.cbegin(); it != title_items.cend(); ++it) {
        iters.push_back(it);
    }
    static auto checked_title_sort = [](const Iterator& it1, const Iterator& it2){
        auto title1 = dynamic_cast<RecordTitleItem*>(it1.value());
        auto title2 = dynamic_cast<RecordTitleItem*>(it2.value());
        if (title1->is_checked() != title2->is_checked()) {
            return title1->is_checked() > title2->is_checked();
        }
        return it1.key() < it2.key();
    };
    static auto title_size_sort = [this](const Iterator& it1, const Iterator& it2){
        auto title1 = dynamic_cast<RecordTitleItem*>(it1.value());
        auto title2 = dynamic_cast<RecordTitleItem*>(it2.value());
        return title1->title_records_size() > title2->title_records_size();
    };
    if (ui->titles_order->currentIndex() == 1) {
        std::sort(iters.begin(), iters.end(), checked_title_sort);
    } else if (ui->titles_order->currentIndex() == 2) {
        std::sort(iters.begin(), iters.end(), title_size_sort);
    }
    int i = 0;
    for (const auto iter : iters) {
        auto title_item = title_items[iter.key()];
        QtConcurrent::run(title_item, &RecordBase::load_thumbmnail);
        ui->title_grid->addWidget(title_item, i/columns, i%columns, Qt::AlignCenter);
        title_item->show();
        ++i;
    }
}

void AbstractOperationMode::update_filters(FilterType type, const QString& text) {
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
        } else if (type == FilterType::DATE) {
            // Handling date filter
            filter(records_by_date(ui->last_used_days->value()));
        } else if (type & FilterType::SINGLE) {
            // Handling size filter
            filter(records_by_size(type));
        } else if (type == FilterType::LOGS) {
            // Handling log filter, overriding any others
            apply_first_filter();
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

void AbstractOperationMode::apply_first_filter() {
    filtration_results.clear();
    bool logs_mode_on = filters.contains("logs") && filters["logs"] == FilterType::LOGS;
    // Log filters overrides any others
    auto i = logs_mode_on ? filters.find("logs") : filters.begin();
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
    } else if (type & FilterType::SINGLE) {
        // Size filter
        for (int index : records_by_size(type)) {
            filtration_results.insert(index, record_items[index]);
        }
    } else if (type == FilterType::LOGS) {
        // Logs watching filter, using unix time as keys for results
        for (int id : logs.keys()) {
            if (records_by_photo_ids.contains(id)) {
                filtration_results.insert(logs[id], record_items[records_by_photo_ids[id]]);
            }
        }
    }
}

void AbstractOperationMode::apply_filters() {
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
            } else if (type & FilterType::SINGLE) {
                // Handling size filter
                filter(records_by_size(type));
            }
        }
    }
}

void AbstractOperationMode::filter(const QSet<int>& second) {
    if (filtration_results.isEmpty()) return;
    QMap<int, RecordBase*> result;
    auto current_results = filtration_results.keys();
    auto keys = QSet<int>(current_results.begin(), current_results.end()).intersect(second);
    for (const auto& key : keys) {
        result.insert(key,record_items[key]);
    }
    filtration_results = result;
}

void AbstractOperationMode::show_filtering_results() {
    ui->slider->setValue(filtration_results.begin().key());
    ui->ok->setEnabled(filtration_results.size() > 1);
    ui->back->setDisabled(true);
    QSet<int> filtration_keys = filtration_results.keys().toSet();
    // Enabling buttons for possible non-zero result filters
    for (int index : filtration_keys) {
        for (const auto& tag : hashtags_by_index[index]) {
            auto hashtag = tag.right(tag.size() - 1);
            hashtags[hashtag]->setEnabled(true);
        }
    }
    // Updating numbers on enabled buttons
    for (auto hashtag : hashtags.keys()) {
        if (hashtags[hashtag]->isEnabled()) {
            hashtags[hashtag]->show_filtered_count(filtration_keys);
            // Concurrent variant:
            // QtConcurrent::run(hashtags[hashtag], &HashtagButton::show_filtered_count, filtration_keys);
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

void AbstractOperationMode::exit_filtering() {
    filtration_results.clear();
    for (auto button : hashtags) {
        button->setEnabled(true);
        button->setChecked(false);
        button->show_count();
    }
    ui->ok->setEnabled(pic_index < records.size() - 1);
    ui->back->setEnabled(pic_index > 0);
    ui->text->setEnabled(true);
    ui->slider->setEnabled(true);
    ui->statusBar->showMessage(QString("Фильтры сняты, всего записей: %1").arg(records.size()));
}

QString AbstractOperationMode::filtration_message(int i) const {
    int j = filters.size();
    return QString("%1 %2 %3 по %4 %5").arg(inflect(i, "Найдено")).arg(i).arg(inflect(i, "записей")).arg(j).arg(inflect(j, "фильтрам"));
}

QString AbstractOperationMode::filtration_indices() const {
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

QString AbstractOperationMode::path(int index) {
    return locations[SCREENSHOTS] + title_name(index) + QDir::separator();
}

QString AbstractOperationMode::series_name(int index) {
    if (series_map.empty()) return QString();
    if (series_map.contains(index)) return series_map.value(index);
    series_map[index] = QString();
    auto it = series_map.find(index);
    auto series = (--it).value();
    series_map.remove(index);
    return series;
}

QPair<int, int> AbstractOperationMode::title_range(int index) {
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

QSet<int> AbstractOperationMode::word_search(const QString& text) {
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

void AbstractOperationMode::check_titles(bool enable) {
    for (auto item : title_items) {
        dynamic_cast<RecordTitleItem*>(item)->set_checked(enable);
    }
}

QSet<int> AbstractOperationMode::checked_title_records() {
    QSet<int> result;
    for (auto title_item : title_items) {
        RecordTitleItem* item = dynamic_cast<RecordTitleItem*>(title_item);
        if (item->is_checked()) {
            result += item->indices();
        }
    }
    return result;
}

QSet<int> AbstractOperationMode::records_by_public(bool publ) {
    QSet<int> result;
    for (int i = 0; i < records.size(); ++i) {
        if (records[i].is_public == publ) {
            result.insert(i);
        }
    }
    return result;
}

QSet<int> AbstractOperationMode::records_by_date(int days) {
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

QSet<int> AbstractOperationMode::records_by_size(FilterType type) {
    if (!(type & SINGLE)) return QSet<int>();
    QSet<int> result;
    for (int i = 0; i < records.size(); ++i) {
        if ((type == SINGLE && records[i].pics.size() == 1) || (type == MULTIPLE && records[i].pics.size() > 1)) {
            result.insert(i);
        }
    }
    return result;
}
