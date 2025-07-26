#include "record_poll_preview.h"
#include "records_dialog.h"
#include <include/database.h>

RecordPollPreview::RecordPollPreview(const RecordPreviewInfo &record, const HashtagPairInfo& tags, const QDateTime& time, QWidget *parent)
    : RecordPreviewBase(record, time, parent), m_tag_info(std::move(tags))
{
    setupUi(this);
    update();
    update_time();
    update_tags();
    connect(pbTime, &QPushButton::clicked, this, &RecordPollPreview::time_dialog);
    connect(pbSearch, &QPushButton::clicked, this, &RecordPollPreview::search);
    connect(pbSwitchUp, &QPushButton::clicked, this, &RecordPollPreview::switch_up);
    connect(pbSwitchDown, &QPushButton::clicked, this, &RecordPollPreview::switch_down);
    connect(sbIndex, QOverload<int>::of(&QSpinBox::valueChanged), this, &RecordPollPreview::spinbox_changed);
}

RecordPollPreview::~RecordPollPreview() {
    if (m_next)
        m_next->m_prev = m_prev;
    if (m_prev)
        m_prev->m_next = m_next;
}

void RecordPollPreview::set_index(int id) {
    QSqlQuery query;
    Database::instance().select_record_by_id(query, id);
    if (query.next()) {
        m_info = RecordPreviewInfo(query);
        update();
    }
}

void RecordPollPreview::set_record(const RecordPreviewInfo &record) {
    m_info = record;
    update();
}

void RecordPollPreview::set_hashtags(const HashtagPairInfo & hashtags) {
    m_tag_info = hashtags;
    update_tags();
}

void RecordPollPreview::set_time(const QDateTime & time) {
    m_time = time;
    update_time();
}

void RecordPollPreview::update_log_info() {
    auto font = lblLogs->font();
    if (m_info.date.isValid()) {
        int days = m_info.date.daysTo(m_time);
        lblLogs->setText(QString("Публиковалось %1 %2 назад").arg(days).arg(inflect(days, "дней")));
        font.setBold(true);
        font.setItalic(false);
    } else {
        lblLogs->setText(QString("Раньше не публиковалось"));
        font.setBold(false);
        font.setItalic(true);
    }
    lblLogs->setFont(font);
}

void RecordPollPreview::reset_spinbox() {
    sbIndex->blockSignals(true);
    sbIndex->setValue(1);
    sbIndex->blockSignals(false);
}

void RecordPollPreview::switch_up() {
    if (!m_prev) return;
    std::swap(m_info, m_prev->m_info);
    sbIndex->blockSignals(true);
    m_prev->sbIndex->blockSignals(true);
    std::swap(m_tag_info, m_prev->m_tag_info);
    int i = sbIndex->value();
    int j = m_prev->sbIndex->value();
    update_tags();
    m_prev->update_tags();

    // Смена индексов в спинбоксе
    sbIndex->setValue(j);
    m_prev->sbIndex->setValue(i);
    sbIndex->blockSignals(false);
    m_prev->sbIndex->blockSignals(false);

    update();
    m_prev->update();
}

void RecordPollPreview::switch_down() {
    if (!m_next) return;
    std::swap(m_info, m_next->m_info);
    sbIndex->blockSignals(true);
    m_next->sbIndex->blockSignals(true);
    std::swap(m_tag_info, m_next->m_tag_info);
    int i = sbIndex->value();
    int j = m_next->sbIndex->value();
    update_tags();
    m_next->update_tags();

    // Смена индексов в спинбоксе
    sbIndex->setValue(j);
    m_next->sbIndex->setValue(i);
    sbIndex->blockSignals(false);
    m_next->sbIndex->blockSignals(false);

    update();
    m_next->update();
}

void RecordPollPreview::search() {
    int tag1 = m_tag_info.tags.first.id;
    int tag2 = m_tag_info.tags.second.id;
    QueryFilters::instance().hashtags.included.insert(tag1, HashtagFilter(tag1));
    QueryFilters::instance().hashtags.included.insert(tag2, HashtagFilter(tag2));
    RecordsDialog dialog(this);
    if (dialog.exec() == QDialog::Accepted) {
        int id = dialog.results().id;
        for (int i = 0; i < m_tag_info.record_ids.size(); ++i)
            if (m_tag_info.record_ids[i] == id)
                sbIndex->setValue(i + 1);
    }
    QueryFilters::instance().hashtags.included.remove(tag1);
    QueryFilters::instance().hashtags.included.remove(tag2);
}

void RecordPollPreview::time_dialog() {
    TimeInputDialog dialog(m_time.time(), this);
    if (dialog.exec() == QDialog::Accepted) {
        m_time.setTime(dialog.selectedTime());
        update_time();
    }
}

void RecordPollPreview::spinbox_changed(int value) {
    set_index(m_tag_info.record_ids[value - 1]);
}

void RecordPollPreview::update() {
    lblNumber->setText(QString().setNum(m_info.id));
    update_text();
    update_images();
    update_log_info();
}

void RecordPollPreview::update_time() {
    pbTime->setToolTip(QString("Время публикации: %1").arg(m_time.time().toString("hh:mm")));
}

void RecordPollPreview::update_images() {
    // Removing images from the end if images.size() > links.size()
    for (int j = m_images.size(); j > m_info.links.size(); --j) {
        m_images.pop_back();
    }
    for (int i = 0; i < m_info.links.size(); ++i) {
        if (m_images.size() <= i) {
            // Adding new image
            m_images.push_back(QSharedPointer<RecordFrame>(new RecordFrame(m_info.links[i])));
            hlImages->addWidget(m_images.back().get());
        } else {
            m_images[i].data()->set_image(m_info.links[i]);
        }
    }
    hlImages->setAlignment(m_info.links.size() > 1 ? Qt::AlignLeft : Qt::AlignHCenter);
}

void RecordPollPreview::update_text() {
    lblText->setText(m_info.quote + '\n' + m_info.hashtags);
    lblTitle->setText("\t" + m_info.title);
}

void RecordPollPreview::update_tags() {
    int max = m_tag_info.record_ids.size();
    sbIndex->setMaximum(max);
    sbIndex->setSuffix(QString("/%1").arg(max));
    lblHashtags->setText(QString("\t%1 + %2").arg(m_tag_info.tags.first.name, m_tag_info.tags.second.name));
}
