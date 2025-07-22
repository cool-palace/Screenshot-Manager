#include "hashtag_poll_button.h"
#include <QMouseEvent>

HashtagPollButton::HashtagPollButton(const HashtagInfo &info, QWidget *parent)
    : m_info(info)
{
    setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Expanding);
    show_count();
}

void HashtagPollButton::mousePressEvent(QMouseEvent *e) {
    if (e->button() == Qt::LeftButton ) {
        emit selected(m_info);
    }
}

void HashtagPollButton::show_count() {
    if (count()) {
        setEnabled(true);
        setText(QString("%1 %2").arg(name()).arg(count()));
        setFlat(false);
    } else {
        setText(name());
        setFlat(true);
    }
}
