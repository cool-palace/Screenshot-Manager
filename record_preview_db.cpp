#include "record_preview_db.h"
#include "records_dialog.h"
#include <QSqlQuery>

RecordPreviewDB::RecordPreviewDB(const RecordPreviewInfo& record, const QDateTime& time, QWidget *parent) :
    QWidget(parent), m_info(std::move(record)), m_time(time)
{
    setupUi(this);
    pbSelect->hide();
    update();
    update_time();
    connect(pbReroll, &QPushButton::clicked, this, &RecordPreviewDB::reroll);
    connect(pbTime, &QPushButton::clicked, this, &RecordPreviewDB::set_time);
    connect(pbSearch, &QPushButton::clicked, this, &RecordPreviewDB::search);
    connect(pbNumber, &QPushButton::clicked, this, &RecordPreviewDB::input_number);
    connect(pbSwitchUp, &QPushButton::clicked, this, &RecordPreviewDB::switch_up);
    connect(pbSwitchDown, &QPushButton::clicked, this, &RecordPreviewDB::switch_down);
}

RecordPreviewDB::~RecordPreviewDB() {
    if (m_next)
        m_next->m_prev = m_prev;
    if (m_prev)
        m_prev->m_next = m_next;
}

void RecordPreviewDB::set_index(int id) {
    QSqlQuery query;
    Database::instance().select_record_by_id(query, id);
    if (query.next()) {
        m_info = RecordPreviewInfo(query);
        update();
    }
}

void RecordPreviewDB::set_record(const RecordPreviewInfo &record) {
    m_info = record;
    update();
}

void RecordPreviewDB::update_log_info() {
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

void RecordPreviewDB::switch_up() {
    if (!m_prev) return;
    std::swap(m_info, m_prev->m_info);
    update();
    m_prev->update();
}

void RecordPreviewDB::switch_down(){
    if (!m_next) return;
    std::swap(m_info, m_next->m_info);
    update();
    m_next->update();
}

void RecordPreviewDB::search() {
    RecordsDialog dialog(this);
    if (dialog.exec() == QDialog::Accepted) {
        set_record(dialog.results());
    }
}

void RecordPreviewDB::set_time() {
    TimeInputDialog dialog(m_time.time(), this);
    if (dialog.exec() == QDialog::Accepted) {
        m_time.setTime(dialog.selectedTime());
        update_time();
    }
}

void RecordPreviewDB::reroll() {
    QSqlQuery query;
    Database::instance().select_records(query, QueryFilters::instance(), true, 1);
    if (query.next()) {
        m_info = RecordPreviewInfo(query);
        update();
    }
}

void RecordPreviewDB::input_number() {
    int max = Database::instance().count_records();
    bool ok;
    int index = QInputDialog::getInt(this, tr("Номер записи"),
                                 tr("Введите номер записи от 1 до %1").arg(max), m_info.id, 1, max, 1, &ok);
    if (!ok) return;
    set_index(index);
}

void RecordPreviewDB::update() {
    lblNumber->setText(QString().setNum(m_info.id));
    update_text();
    update_images();
    update_log_info();
}

void RecordPreviewDB::update_time() {
    pbTime->setToolTip(QString("Время публикации: %1").arg(m_time.time().toString("hh:mm")));
}

void RecordPreviewDB::update_images() {
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

void RecordPreviewDB::update_text() {
    lblText->setText(m_info.quote + '\n' + m_info.hashtags);
    lblTitle->setText("\t" + m_info.title);
}
