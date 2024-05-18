#include "hashtag_button.h"

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
    setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Expanding);
    setToolTip(text);
    setCheckable(true);
}

void HashtagButton::mousePressEvent(QMouseEvent * e) {
    if (preview_to_change) return;
    switch (e->button()) {
    case Qt::LeftButton:
        if (e->modifiers() & Qt::ShiftModifier) {
            emit filterEvent(FilterType::HASHTAG, text);
        } else if (e->modifiers() & Qt::AltModifier) {
            emit filterEvent(FilterType::HASHTAG_EXCLUDE, text);
        } else {
            emit hashtagEvent('#', text);
        }
        break;
    case Qt::RightButton:
        if (e->modifiers() & Qt::ShiftModifier) {
            emit filterEvent(FilterType::AMPTAG, text);
        } else if (e->modifiers() & Qt::AltModifier) {
            emit filterEvent(FilterType::AMPTAG_EXCLUDE, text);
        } else {
            emit hashtagEvent('&', text);
        }
        break;
    case Qt::MiddleButton:
        if (e->modifiers() & Qt::AltModifier) {
            emit filterEvent(FilterType::ANY_TAG_EXCLUDE, text);
        } else {
            emit filterEvent(FilterType::ANY_TAG, text);
        }
        break;
    default:
        break;
    }
}

void HashtagButton::mouseDoubleClickEvent(QMouseEvent* e) {
    if (!preview_to_change) return;
    if (e->button() == Qt::LeftButton ) {
        emit selected(text, preview_to_change);
    }
}

void HashtagButton::show_count() {
    setText(count ? text + ' ' + QString().setNum(count) : text);
    setFlat(count);
}

void HashtagButton::reset() {
    count = 0;
    record_indices.clear();
    setChecked(false);
    setEnabled(true);
    show_count();
}

void HashtagButton::add_index(const QChar& sign, int index) {
    record_indices[sign].insert(index);
    ++count;
}

void HashtagButton::remove_index(const QChar& sign, int index) {
    record_indices[sign].remove(index);
    --count;
}

QSet<int> HashtagButton::indices(const QChar& sign, bool include) const {
    auto set = sign == ' '
             ? record_indices['#'] + record_indices['&']
             : record_indices[sign];
    if (include) return set;
    QSet<int> result;
    for (int i = 0; i < records_size; ++i) {
        if (!set.contains(i)) result.insert(i);
    }
    return result;
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
