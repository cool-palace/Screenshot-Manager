#include "hashtag_button.h"

Hashtag::Hashtag(const QString& name, const QJsonObject& object) : name(name) {
    rank = object["rank"].toInt();
    poll = QDateTime::fromSecsSinceEpoch(object["last_poll"].toInt(), Qt::LocalTime);
    won = QDateTime::fromSecsSinceEpoch(object["last_won"].toInt(), Qt::LocalTime);
    description = object["description"].toString();
    emoji = object["emoji"].toString();
}

QString Hashtag::text() const {
    QString tag = name;
    return tag.replace(0, 1, name[0].toUpper()) + " — " + description;
}

QString Hashtag::option() const {
    return emoji + " " + name;
}

HashtagPreview::HashtagPreview(const Hashtag& tag) : QWidget(), index(total++), hashtag(tag) {
    text.setText(hashtag.text());
    text.setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Fixed);
    text.setMaximumHeight(120);
    text.setWordWrap(true);
    auto font = text.font();
    font.setPointSize(12);
    text.setFont(font);
//    text.hide();
    number.setSizePolicy(QSizePolicy::Fixed, QSizePolicy::Fixed);
    number.setMinimumWidth(20);
    number.setFont(font);
    number.setText(QString().setNum(index));
//    number.hide();
    layout.setContentsMargins(0,0,0,0);
    setLayout(&layout);
    layout.addWidget(&number,0,0);
    layout.addWidget(&text,0,1);
}

int HashtagPreview::total = 0;

HashtagButton::HashtagButton(const QString& text) :
    QPushButton(text),
    text(text)
{
    setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Expanding);
    setToolTip(text);
    setCheckable(true);
}

void HashtagButton::mousePressEvent(QMouseEvent * e) {
    switch (e->button()) {
    case Qt::LeftButton:
        if (e->modifiers() & Qt::ShiftModifier) {
            emit filterEvent('#', text, true);
        } else if (e->modifiers() & Qt::AltModifier) {
            emit filterEvent('#', text, false);
        } else {
            emit hashtagEvent('#', text);
        }
        break;
    case Qt::RightButton:
        if (e->modifiers() & Qt::ShiftModifier) {
            emit filterEvent('&', text, true);
        } else if (e->modifiers() & Qt::AltModifier) {
            emit filterEvent('&', text, false);
        } else {
            emit hashtagEvent('&', text);
        }
        break;
    case Qt::MiddleButton:
        if (e->modifiers() & Qt::AltModifier) {
            emit filterEvent(' ', text, false);
        } else {
            emit filterEvent(' ', text, true);
        }
        break;
    default:
        break;
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

void HashtagButton::highlight(const QChar& sign, bool enable) {
    auto _font = font();
    if (sign == '#') {
        _font.setBold(enable);
    } else if (sign == '&') {
        _font.setItalic(enable);
    }
    setFont(_font);
}

void HashtagButton::highlight(bool include, bool enable) {
    auto _font = font();
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

void HashtagButton::update_on_records(int size) {
    records_size = size;
}
