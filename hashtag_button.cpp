#include "hashtag_button.h"

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
            emit filterEvent('#', text, true);
        } else if (e->modifiers() & Qt::AltModifier) {
            emit filterEvent('#', text, false);
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
    return include ? set : all_records.subtract(set);
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

QSet<int> HashtagButton::all_records;

void HashtagButton::update_on_records(int size) {
    all_records.clear();
    for (int i = 0; i < size; ++i) {
        all_records.insert(i);
    }
}
