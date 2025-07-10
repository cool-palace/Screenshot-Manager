#include "records_dialog.h"
#include <include/database.h>
#include <include/common.h>

RecordsDialog::RecordsDialog(QWidget *parent) : QDialog(parent) {
    setupUi(this);
    setWindowTitle("Выбор записей");
    m_count = Database::instance().count_records(QueryFilters::instance());
    update();
    connect(sbPage, QOverload<int>::of(&QSpinBox::valueChanged), this, &RecordsDialog::update);
    connect(sbLimit, QOverload<int>::of(&QSpinBox::valueChanged), this, &RecordsDialog::update);

    setWindowState(Qt::WindowMaximized);
}

void RecordsDialog::update() {
    sbPage->setMaximum(m_count / sbLimit->value() + 1);

    QList<RecordPreviewInfo> record_infos;
    QSqlQuery query;
    Database::instance().select_records(query, QueryFilters::instance(), false, sbLimit->value(), offset());
    while (query.next()) {
        record_infos.append(RecordPreviewInfo(query));
    }

    // Удаляем лишние виджеты, если их существует больше нужного
    for (int j = m_records.size(); j > record_infos.size(); --j) {
        delete m_records.back();
        m_records.pop_back();
    }
    for (int i = 0; i < record_infos.size(); ++i) {
        if (m_records.size() <= i) {
            // Если меньше нужного, добавляем новый виджет
            RecordItemDB* record_item = new RecordItemDB(record_infos[i], this);
            connect(record_item, &RecordItemDB::selected, [this](const RecordPreviewInfo& record) {
                m_result = std::move(record);
                accept();
            });
            m_records.append(record_item);
            grlRecords->addWidget(record_item);
        } else {
            // Обновляем уже существующие виджеты
            m_records[i]->set_record(std::move(record_infos[i]));
        }
    }
}
