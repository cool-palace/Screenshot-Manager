#include "hashtag_button_db.h"
#include <QMouseEvent>

HashtagButtonDB::HashtagButtonDB(const HashtagInfo &info, QWidget *parent)
    : m_info(info)
{
    QAction * action_include_all = m_menu.addAction("Выбрать все записи");
    QAction * action_include_hash = m_menu.addAction("Выбрать со знаком #");
    QAction * action_include_amp = m_menu.addAction("Выбрать со знаком &&");
    QAction * action_exclude_all = m_menu.addAction("Исключить все записи");
    QAction * action_exclude_hash = m_menu.addAction("Исключить со знаком #");
    QAction * action_exclude_amp = m_menu.addAction("Исключить со знаком &&");
    action_include_all->setCheckable(true);
    action_include_hash->setCheckable(true);
    action_include_amp->setCheckable(true);
    action_exclude_all->setCheckable(true);
    action_exclude_hash->setCheckable(true);
    action_exclude_amp->setCheckable(true);
    connect(action_include_all, &QAction::triggered, [this]() {
        select_action(HashtagActions::INCLUDE_ALL);
        emit filterEvent(HashtagFilterType::ANY_TAG, id());
    });
    connect(action_include_hash, &QAction::triggered, [this]() {
        select_action(HashtagActions::INCLUDE_HASH);
        emit filterEvent(HashtagFilterType::HASHTAG, id());
    });
    connect(action_include_amp, &QAction::triggered, [this]() {
        select_action(HashtagActions::INCLUDE_AMP);
        emit filterEvent(HashtagFilterType::AMPTAG, id());
    });
    connect(action_exclude_all, &QAction::triggered, [this]() {
        select_action(HashtagActions::EXCLUDE_ALL);
        emit filterEvent(HashtagFilterType::ANY_TAG_EXCLUDE, id());
    });
    connect(action_exclude_hash, &QAction::triggered, [this]() {
        select_action(HashtagActions::EXCLUDE_HASH);
        emit filterEvent(HashtagFilterType::HASHTAG_EXCLUDE, id());
    });
    connect(action_exclude_amp, &QAction::triggered, [this]() {
        select_action(HashtagActions::EXCLUDE_AMP);
        emit filterEvent(HashtagFilterType::AMPTAG_EXCLUDE, id());
    });
    setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Expanding);
    setCheckable(true);
    show_count();
}

void HashtagButtonDB::mousePressEvent(QMouseEvent *e) {
    switch (e->button()) {
    case Qt::RightButton:
        m_menu.exec(e->globalPos());
        break;
    case Qt::MiddleButton: case Qt::LeftButton: {
        // Если какое-то действие уже выбрано, то выберем его, чтобы сбросить
        // Если действие не выбрано, то выбираем первый вариант, чтобы выбрать все результаты по тегу
        HashtagActions action = qMax(m_selected_action, HashtagActions::INCLUDE_ALL);
        m_menu.actions().at(static_cast<int>(action))->trigger();
        break; }
    default:
        break;
    }
}

void HashtagButtonDB::highlight(const HashtagActions action, bool enable) {
    auto _font = font();
    bool include = action > HashtagActions::NO_ACTION && action < HashtagActions::EXCLUDE_ALL;
    if (include) {
        _font.setUnderline(enable);
    } else {
        _font.setStrikeOut(enable);
    }
    if (action == HashtagActions::INCLUDE_HASH || action == HashtagActions::EXCLUDE_HASH)
        _font.setBold(enable);
    else if (action == HashtagActions::INCLUDE_AMP || action == HashtagActions::EXCLUDE_AMP)
        _font.setItalic(enable);
    setFont(_font);
    setChecked(enable);
    setEnabled(true);
}

void HashtagButtonDB::set_count(int count) {
    m_info.count = count;
    show_count();
}

void HashtagButtonDB::show_count() {
    if (count()) {
        setEnabled(true);
        setText(QString("%1 %2").arg(name()).arg(count()));
        setFlat(false);
    } else {
        setText(name());
        setFlat(true);
    }
}

void HashtagButtonDB::trigger(HashtagActions action) {
    int index = static_cast<int>(action);
    m_menu.actions().at(index)->trigger();
}

void HashtagButtonDB::set_filter(const bool include, const HashtagFilter filter) {
    switch (filter.code()) {
    case HashtagFilter::Code::BOTH:
        trigger(include ? HashtagActions::INCLUDE_ALL : HashtagActions::EXCLUDE_ALL);
        break;
    case HashtagFilter::Code::HASHTAG:
        trigger(include ? HashtagActions::INCLUDE_HASH : HashtagActions::EXCLUDE_HASH);
        break;
    case HashtagFilter::Code::AMPTAG:
        trigger(include ? HashtagActions::INCLUDE_AMP : HashtagActions::EXCLUDE_AMP);
        break;
    }
    // Включаем кнопку, если есть результаты или задан исключающий фильтр
    setEnabled(m_info.count > 0 || !include);
}

void HashtagButtonDB::select_action(HashtagActions action) {
    if (m_selected_action == HashtagActions::NO_ACTION)
        m_selected_action = action;
    else if (m_selected_action == action)
        m_selected_action = HashtagActions::NO_ACTION;
    else return;

    const bool active_action = m_selected_action != HashtagActions::NO_ACTION;
    setChecked(active_action);
    highlight(action, active_action);

    // Если выбрано какое-то действие
    if (m_selected_action != HashtagActions::NO_ACTION) {
        for (int i = 0; i < m_menu.actions().size(); ++i) {
            if (static_cast<int>(m_selected_action) == i) {
                m_menu.actions().at(i)->setEnabled(true);
                m_menu.actions().at(i)->setChecked(true);
            } else {
                m_menu.actions().at(i)->setEnabled(false);
            }
        }
    // Иначе восстанавливаем исходное состояние
    } else for (int i = 0; i < m_menu.actions().size(); ++i) {
        m_menu.actions().at(i)->setEnabled(true);
        m_menu.actions().at(i)->setChecked(false);
    }
}
