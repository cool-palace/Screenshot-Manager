#include "include\hashtag_button.h"

Hashtag::Hashtag(const QString& name, const QJsonObject& object) : name(name) {
    rank = object["rank"].toInt();
    poll = QDateTime::fromSecsSinceEpoch(object["last_poll"].toInt(), Qt::LocalTime);
    won = QDateTime::fromSecsSinceEpoch(object["last_won"].toInt(), Qt::LocalTime);
    descr = object["description"].toString();
    emoji = object["emoji"].toString();
}

QString Hashtag::text() const {
    QString tag = name;
    return tag.replace(0, 1, name[0].toUpper()) + " — ";
}

QString Hashtag::option() const {
    return QString('\"') + emoji + " " + name + '\"';
}

QJsonObject Hashtag::to_json() const {
    QJsonObject current_hashtag;
    current_hashtag["rank"] = rank;
    current_hashtag["description"] = descr;
    current_hashtag["emoji"] = emoji;
    return current_hashtag;
}

HashtagPreview::HashtagPreview(const Hashtag& tag) : QWidget() {
    text.setSizePolicy(QSizePolicy::Fixed, QSizePolicy::Fixed);
    text.setMinimumWidth(200);
    text.setMaximumHeight(120);
    text.setWordWrap(true);
    auto font = text.font();
    font.setPointSize(12);
    text.setFont(font);
    number.setSizePolicy(QSizePolicy::Fixed, QSizePolicy::Fixed);
    number.setMinimumWidth(20);
    number.setFont(font);
    number.setText(QString().setNum(index));
    log_info.setFont(text.font());
    description.setFont(font);
    description.setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Fixed);
    layout.setContentsMargins(0,0,0,0);
    setLayout(&layout);
    layout.addWidget(&number,0,0);
    layout.addWidget(&text,0,1);
    layout.addWidget(&description,0,2);
    layout.addWidget(&log_info,1,1);
    layout.addWidget(reroll_button,0,3);
    layout.addWidget(search_button,0,4);
    layout.addWidget(check_button,0,5);
    connect(reroll_button, &QPushButton::clicked, this, &HashtagPreview::reroll);
    connect(search_button, &QPushButton::clicked, this, &HashtagPreview::search);
    connect(check_button, &QPushButton::clicked, this, &HashtagPreview::check);
    reroll_button->setIconSize(QSize(30,30));
    search_button->setIconSize(QSize(30,30));
    check_button->setIconSize(QSize(30,30));
    connect(&description, &QLineEdit::editingFinished, [this]() { edited = true; });
    //    set_hashtag(tag);
}

HashtagPreview::~HashtagPreview() {
    disconnect(reroll_button, nullptr, this, nullptr);
    disconnect(search_button, nullptr, this, nullptr);
    disconnect(check_button, nullptr, this, nullptr);
    disconnect(&description, nullptr, this, nullptr);
    delete reroll_button;
    delete search_button;
    delete check_button;
}

QMap<QString, int>* HashtagPreview::poll_logs;

void HashtagPreview::update_log_info() {
    auto font = log_info.font();
    if (poll_logs->contains(hashtag.tag())) {
        QDateTime last = QDateTime::fromSecsSinceEpoch(poll_logs->value(hashtag.tag()), Qt::LocalTime);
        log_info.setText(QString("Публиковалось %1 дней назад").arg(last.daysTo(QDateTime::currentDateTime())));
        font.setBold(true);
        font.setItalic(false);
    } else {
        log_info.setText(QString("Раньше не публиковалось"));
        font.setBold(false);
        font.setItalic(true);
    }
    log_info.setFont(font);
}

void HashtagPreview::update_count(int count) {
    number.setText(QString().setNum(count));
}

void HashtagPreview::reroll() {
    emit reroll_request(hashtag.tag());
}

void HashtagPreview::search() {
    emit search_start(hashtag.tag());
}

void HashtagPreview::check() {
    emit check_request(hashtag.tag());
}

void HashtagPreview::set_hashtag(const Hashtag& tag) {
    hashtag = tag;
    text.setText(hashtag.text());
    description.setText(hashtag.description());
    edited = false;
    update_log_info();
    emit count_request(hashtag.tag());
}

HashtagButton::HashtagButton(const QString& text) :
    QPushButton(text),
    text(text)
{
    QAction * action_add_amp = menu.addAction("Добавить со знаком &&");
    QAction * action_include_all = menu.addAction("Выбрать все записи");
    QAction * action_include_hash = menu.addAction("Выбрать со знаком #");
    QAction * action_include_amp = menu.addAction("Выбрать со знаком &&");
    QAction * action_exclude_all = menu.addAction("Исключить все записи");
    QAction * action_exclude_hash = menu.addAction("Исключить со знаком #");
    QAction * action_exclude_amp = menu.addAction("Исключить со знаком &&");
    action_add_amp->setCheckable(true);
    action_include_all->setCheckable(true);
    action_include_hash->setCheckable(true);
    action_include_amp->setCheckable(true);
    action_exclude_all->setCheckable(true);
    action_exclude_hash->setCheckable(true);
    action_exclude_amp->setCheckable(true);
    connect(action_add_amp, &QAction::triggered, [this, action_add_amp, text]() {
        select_action(action_add_amp);
        emit hashtagEvent('&', text);
    });
    connect(action_include_all, &QAction::triggered, [this, action_include_all, text]() {
        select_action(action_include_all);
        emit filterEvent(FilterType::ANY_TAG, text);
    });
    connect(action_include_hash, &QAction::triggered, [this, action_include_hash, text]() {
        select_action(action_include_hash);
        emit filterEvent(FilterType::HASHTAG, text);
    });
    connect(action_include_amp, &QAction::triggered, [this, action_include_amp, text]() {
        select_action(action_include_amp);
        emit filterEvent(FilterType::AMPTAG, text);
    });
    connect(action_exclude_all, &QAction::triggered, [this, action_exclude_all, text]() {
        select_action(action_exclude_all);
        emit filterEvent(FilterType::ANY_TAG_EXCLUDE, text);
    });
    connect(action_exclude_hash, &QAction::triggered, [this, action_exclude_hash, text]() {
        select_action(action_exclude_hash);
        emit filterEvent(FilterType::HASHTAG_EXCLUDE, text);
    });
    connect(action_exclude_amp, &QAction::triggered, [this, action_exclude_amp, text]() {
        select_action(action_exclude_amp);
        emit filterEvent(FilterType::AMPTAG_EXCLUDE, text);
    });
    setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Expanding);
    setToolTip(text);
    setCheckable(true);
}

HashtagButton::~HashtagButton() {
    disconnect(this, nullptr, nullptr, nullptr);
}

void HashtagButton::mousePressEvent(QMouseEvent * e) {
    if (preview_to_change) return;
    switch (e->button()) {
    case Qt::LeftButton:
        emit hashtagEvent('#', text);
        break;
    case Qt::RightButton:
        menu.exec(e->globalPos());
        break;
    case Qt::MiddleButton:
        emit filterEvent(FilterType::ANY_TAG, text);
        break;
    default:
        break;
    }
}

void HashtagButton::mouseDoubleClickEvent(QMouseEvent* e) {
    if (!preview_to_change) return;
    if (e->button() == Qt::LeftButton) {
        emit selected(text, preview_to_change);
    }
}

void HashtagButton::show_count() {
    int count = get_count();
    setText(count ? text + ' ' + QString().setNum(count) : text);
    setFlat(count);
}

int HashtagButton::current_count() const {
    if (isEnabled()) {
        bool ok = true;
        int count = QAbstractButton::text().split(' ').back().toInt(&ok);
        return ok ? count : 0;
    } else return 0;
}

void HashtagButton::show_filtered_count(const QSet<int>& results) {
    QSet<int> intersection = all_indices().intersect(results);
    int count = intersection.size();
    setText(count ? text + ' ' + QString().setNum(count) : text);
    setFlat(count);
}

void HashtagButton::reset() {
    record_indices.clear();
    setChecked(false);
    setEnabled(true);
    show_count();
}

void HashtagButton::add_index(const QChar& sign, int index) {
    record_indices[sign].insert(index);
}

void HashtagButton::remove_index(const QChar& sign, int index) {
    record_indices[sign].remove(index);
}

QSet<int> HashtagButton::indices(FilterType type) const {
    QSet<int> set;
    if (!(type & FilterType::EXCLUDE)) {
        set = type == FilterType::ANY_TAG
                 ? record_indices['#'] + record_indices['&']
                 : record_indices[type == FilterType::HASHTAG ? '#' : '&'];
        return set;
    }
    QSet<int> result;
    for (int i = 0; i < records_size; ++i) {
        if (!set.contains(i)) result.insert(i);
    }
    return result;
}

void HashtagButton::highlight(const QChar& sign, bool enable) {
    auto _font = font();
    if (sign == '#') {
        _font.setBold(enable);
    } else if (sign == '&') {
        _font.setItalic(enable);
    }
    setFont(_font);
}

void HashtagButton::highlight(FilterType type, bool enable) {
    auto _font = font();
    bool include = !(type & FilterType::EXCLUDE);
    if (include) {
        _font.setUnderline(enable);
    } else {
        _font.setStrikeOut(enable);
    }
    setFont(_font);
    setChecked(enable);
    setEnabled(true);
}

void HashtagButton::highlight_unregistered() {
    auto _font = font();
    _font.setOverline(true);
    setFont(_font);
    setToolTip("Вероятно, этот тег ошибочный");
    disconnect(SIGNAL(hashtagEvent(const QChar&, const QString&)));
}

int HashtagButton::records_size = 0;
HashtagPreview* HashtagButton::preview_to_change = nullptr;

void HashtagButton::update_on_records(int size) {
    records_size = size;
}

void HashtagButton::set_preview_to_change(HashtagPreview * preview) {
    preview_to_change = preview;
}

HashtagPreview* HashtagButton::current_preview_to_change() {
    return preview_to_change;
}

void HashtagButton::emit_filter_event() {
    emit filterEvent(FilterType::ANY_TAG, text);
}

void HashtagButton::select_action(QAction * action) {
    action_selected = !action_selected;
    for (auto act : menu.actions()) {
        act->setDisabled(action_selected);
    }
    action->setEnabled(true);
    action->setChecked(action_selected);
}
