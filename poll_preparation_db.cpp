#include "hashtag_poll_dialog.h"
#include "locations.h"
#include "poll_preparation_db.h"
#include "posting_progress_dialog.h"
#include <include/database.h>

PollPreparationDB::PollPreparationDB(QWidget *parent) : QWidget(parent) {
    setupUi(this);

    tabWidget->setTabText(0, "Хэштеги");
    tabWidget->setTabText(1, "Посты");

    connect(pbGenHashtags, &QPushButton::clicked, this, &PollPreparationDB::generate_hashtags);
    connect(pbGenPosts, &QPushButton::clicked, this, &PollPreparationDB::generate_posts);
    connect(pbPost, &QPushButton::clicked, this, &PollPreparationDB::post_button);

    connect(grpPublicity, &QGroupBox::clicked, this, &PollPreparationDB::publicity_filter_changed);
    connect(grpLastUsed,  &QGroupBox::clicked, this, &PollPreparationDB::last_used_filter_changed);
    connect(grpPollDays,  &QGroupBox::clicked, this, &PollPreparationDB::poll_filter_changed);
    connect(grpCount,     &QGroupBox::clicked, this, &PollPreparationDB::poll_filter_changed);

    connect(cbPublicity,    QOverload<int>::of(&QComboBox::currentIndexChanged), this, &PollPreparationDB::publicity_filter_changed);

    connect(sbLastUsedDays, QOverload<int>::of(&QSpinBox::valueChanged), this, &PollPreparationDB::last_used_filter_changed);
    connect(sbPollDays,     QOverload<int>::of(&QSpinBox::valueChanged), this, &PollPreparationDB::poll_filter_changed);
    connect(sbCount,        QOverload<int>::of(&QSpinBox::valueChanged), this, &PollPreparationDB::poll_filter_changed);
    connect(sbCycle,        QOverload<int>::of(&QSpinBox::valueChanged), this, &PollPreparationDB::set_cycle);
}

void PollPreparationDB::start() {
    Database::instance().init(Locations::instance()[DATABASE]);
    dteStart->setMinimumDate(QDate::currentDate());
    dteStart->setDate(QTime::currentTime() < QTime(3,0)
                          ? QDate::currentDate()
                          : QDate::currentDate().addDays(1));
    int day = dteStart->date().dayOfWeek();
    dteEnd->setDate(dteStart->date().addDays(day < 4 ? 4 - day : 2)); // По умолчанию устанавливаем четверг или +2 дня
    dteEnd->setTime(QTime(21, 0));
    bool weekend = day > 5;
    dteStart->setTime(QTime(8,0));
    if (weekend) sbSize->setValue(6);

    QSqlQuery query;
    Database::instance().select_hashtag_info(query, QueryFilters::instance());
    while (query.next()) {
        int id = query.value("id").toInt();
        m_hashtag_info.insert(id, HashtagInfo(query));
    }
    set_enabled(true);

    publicity_filter_changed();
    last_used_filter_changed();

    pbGenHashtags->click();
}

void PollPreparationDB::set_enabled(bool enable) {
    dteStart->setEnabled(enable);
    pbGenHashtags->setEnabled(enable);
    teInterval->setEnabled(enable);
    dteStart->setEnabled(enable);
    dteEnd->setEnabled(enable);
    pbPost->setEnabled(enable);
    sbSize->setEnabled(enable);
}

void PollPreparationDB::publicity_filter_changed() {
    QueryFilters::instance().publicity.enabled = grpPublicity->isChecked();
    QueryFilters::instance().publicity.hidden = cbPublicity->currentIndex() > 0;
    update_results();
}

void PollPreparationDB::last_used_filter_changed() {
    sbLastUsedDays->setEnabled(grpLastUsed->isChecked());
    QueryFilters::instance().last_used.enabled = grpLastUsed->isChecked();
    QueryFilters::instance().last_used.date = dteStart->date().addDays(-sbLastUsedDays->value());
    update_results();
}

void PollPreparationDB::poll_filter_changed() {
    QueryFilters::instance().poll_filters.min_date_enabled = grpPollDays->isChecked();
    QueryFilters::instance().poll_filters.min_date = dteStart->date().addDays(-sbPollDays->value());;
    QueryFilters::instance().poll_filters.min_count_enabled = grpCount->isChecked();
    QueryFilters::instance().poll_filters.min_count = sbCount->value();
}

void PollPreparationDB::update_results() {
    int count = Database::instance().count_records(QueryFilters::instance());
    lblRecords->setText(QString("Найдено результатов: %1").arg(count));
}

void PollPreparationDB::hashtag_dialog() {
    HashtagPreviewDB* hashtag = qobject_cast<HashtagPreviewDB*>(sender());
    HashtagPollDialog dialog(m_hashtag_info, m_selected_hashtags, this);
    if (dialog.exec() == QDialog::Accepted) {
        hashtag->set_hashtag(dialog.results());
    }
}

void PollPreparationDB::update_hashtag_count() {
    QSqlQuery query;
    Database::instance().count_hashtags(query, QueryFilters::instance());
    while (query.next()) {
        int id = query.value("id").toInt();
        int count = query.value("count").toInt();
        m_hashtag_info.find(id)->count = count;
    }
}

void PollPreparationDB::prepare_tag_map() {
    for (const QVector<int>& cycle : m_hamiltonian_cycles) {
        QList<int> tag_id_list;
        for (const HashtagPreviewDB* hashtag : m_selected_hashtags)
            tag_id_list.append(hashtag->id());
    }
}

void PollPreparationDB::set_cycle(int value) {
    if (value > m_hamiltonian_cycles.size()) {
        qDebug() << "Выход за границы диапазона циклов";
        return;
    }
    tabWidget->setCurrentIndex(1);
    QList<HashtagPairInfo> pairs = tag_pairs(m_hamiltonian_cycles[value - 1]);
    QList<int> first_record_ids;
    for (const HashtagPairInfo& pair : pairs)
        first_record_ids.append(pair.record_ids.first());

    // Читаем данные записей из базы
    QHash<int, RecordPreviewInfo> info_by_id;
    QSqlQuery query;
    Database::instance().select_records_by_ids(query, first_record_ids);
    while (query.next()) {
        RecordPreviewInfo info(query);
        info_by_id.insert(info.id, info);
    }

    // Затем размножаем их обратно в исходном порядке (страховака на случай совпадения записей)
    QList<RecordPreviewInfo> record_infos;
    record_infos.reserve(first_record_ids.size());
    for (int id : first_record_ids) {
        record_infos.append(info_by_id.value(id));
    }

    // Удаляем лишние виджеты, если их существует больше нужного
    for (int j = m_selected_records.size(); j > record_infos.size(); --j) {
        delete m_selected_records.back();
        m_selected_records.pop_back();
    }

    // Задаём начальное время для постов
    QDateTime time = QDateTime(dteStart->date(), dteStart->time(), Qt::LocalTime);
    time = time.addSecs(teInterval->time().hour()*3600 + teInterval->time().minute()*60);

    RecordPollPreview* previous = nullptr;
    for (int i = 0; i < record_infos.size(); ++i) {
        HashtagPairInfo pair = pairs[i];
        if (m_selected_records.size() <= i) {
            // Если меньше нужного, добавляем новый виджет
            RecordPollPreview* record_preview = new RecordPollPreview(record_infos[i], pairs[i], time, this);
            m_selected_records.append(record_preview);
            if (previous) {
                previous->set_next(record_preview);
                record_preview->set_prev(previous);
            }
            previous = record_preview;
            vlPreview->addWidget(record_preview);
        } else {
            // Обновляем уже существующие виджеты
            m_selected_records[i]->set_record(std::move(record_infos[i]));
            m_selected_records[i]->set_hashtags(pairs[i]);
            m_selected_records[i]->reset_spinbox();
            m_selected_records[i]->set_time(time);
            previous = m_selected_records[i];
        }
        time = time.addSecs(teInterval->time().hour()*3600 + teInterval->time().minute()*60);
    }
}

void PollPreparationDB::display_matrix(const QVector<QVector<int>> &matrix) {
    if (matrix.isEmpty()) return;

    const int size = matrix.size();
    QString result;

    // Строим заголовок
    result += "   ";  // отступ под индексы строк
    for (int col = 0; col < size; ++col)
        result += QString(" %1").arg(col + 1, 2);
    result += '\n';

    for (int row = 0; row < size; ++row) {
        result += QString("%1 ").arg(row + 1, 2);  // индекс строки
        for (int col = 0; col < size; ++col) {
            int val = matrix[row][col];
            QChar symbol = ' ';
            if (row == col) symbol = 'X';
            else if (val == 0) symbol = ' ';
            else if (val == 1) symbol = '.';
            else if (val <= 5) symbol = ':';
            else if (val <= 20) symbol = '*';
            else if (val <= 50) symbol = '+';
            else if (val <= 100) symbol = '#';
            else if (val > 100) symbol = '@';
            result += QString("  %1").arg(symbol);
        }
        result += '\n';
    }
    lblMatrix->setText(result);
}

void PollPreparationDB::generate_hashtags() {
    tabWidget->setCurrentIndex(0);
    QList<HashtagInfo> hashtag_infos;
    QSqlQuery query;
    Database::instance().select_hashtag_info(query, QueryFilters::instance(), true, sbSize->value());
    while (query.next()) {
        hashtag_infos.append(HashtagInfo(query));
    }
    // Удаляем лишние виджеты, если их существует больше нужного
    for (int j = m_selected_hashtags.size(); j > hashtag_infos.size(); --j) {
        delete m_selected_hashtags.back();
        m_selected_hashtags.pop_back();
    }
    for (int i = 0; i < hashtag_infos.size(); ++i) {
        if (m_selected_hashtags.size() <= i) {
            // Если меньше нужного, добавляем новый виджет
            HashtagPreviewDB* hashtag_preview = new HashtagPreviewDB(hashtag_infos[i], this);
            m_selected_hashtags.append(hashtag_preview);
            connect(hashtag_preview, &HashtagPreviewDB::dialog_start, this, &PollPreparationDB::hashtag_dialog);
            connect(hashtag_preview, &HashtagPreviewDB::changed, this, &PollPreparationDB::tag_pairing_analysis);
            vlHashtags->addWidget(hashtag_preview);
        } else {
            // Обновляем уже существующие виджеты
            m_selected_hashtags[i]->set_hashtag(std::move(hashtag_infos[i]));
        }
    }
    tag_pairing_analysis();
}

void PollPreparationDB::tag_pairing_analysis() {
    QList<int> tag_ids;
    tag_ids.reserve(m_selected_hashtags.size());
    for (HashtagPreviewDB* hashtag : m_selected_hashtags)
        tag_ids.append(hashtag->id());

    QSqlQuery query;
    Database::instance().select_hashtag_pairs_count(query, tag_ids, QueryFilters::instance());
    QVector<QVector<int>> tmp_matrix = matrix(query, tag_ids);
    display_matrix(tmp_matrix);
    m_hamiltonian_cycles = get_all_hamiltonian_cycles(tmp_matrix);
    qDebug() << "Hamiltonian cycles:";
    for (const QVector<int>& cycle : m_hamiltonian_cycles) {
        qDebug() << cycle;
    }
    const bool enable_posts = !m_hamiltonian_cycles.empty();
    if (enable_posts) {
        int cycles = m_hamiltonian_cycles.size();
        sbCycle->blockSignals(true);
        sbCycle->setValue(1);
        sbCycle->setMaximum(cycles);
        sbCycle->setSuffix(QString("/%1").arg(cycles));
        sbCycle->blockSignals(false);
        lblCyclesInfo->setText("Комбинация тегов подходит для автоматического подбора постов.");
    } else
        lblCyclesInfo->setText("Комбинация тегов сочетается мало.");
    pbGenPosts->setEnabled(enable_posts);
    sbCycle->setEnabled(enable_posts);
}

void PollPreparationDB::generate_posts() {
    set_cycle(1);
}

void PollPreparationDB::post_button() {
    auto dialog = new PostingProgressDialog(m_selected_hashtags, qMakePair(dteStart->dateTime(), dteEnd->dateTime()), m_selected_records, this);
    dialog->show();
    dialog->start_posting();
}

QVector<QVector<int>> PollPreparationDB::matrix(QSqlQuery &query, const QList<int>& tag_ids) {
    m_tags_records_map.clear();
    int size = tag_ids.size();
    // Инициализируем матрицу нулями
    QVector<QVector<int>> result(size, QVector<int>(size, 0));
    // Словарь для перевода tag_id в индексе
    QMap<int, int> tag_id_to_index;
    for (int i = 0; i < size; ++i)
        tag_id_to_index[tag_ids[i]] = i;
    while (query.next()) {
        int tag1 = query.value("tag1").toInt();  // tag_id_1
        int tag2 = query.value("tag2").toInt();  // tag_id_2
        int count = query.value("count").toInt(); // количество записей с этой парой

        int i = tag_id_to_index.value(tag1, -1);
        int j = tag_id_to_index.value(tag2, -1);

        if (i != -1 && j != -1) {
            result[i][j] = count;
            result[j][i] = count;
        }
        // Сохраняем список записей для пары тегов
        QStringList record_ids = query.value("records").toString().split(',');
        QSet<int> tags;
        tags << tag1 << tag2;
        QList<int> records;
        records.reserve(count);
        for (const QString& id : record_ids)
            records.append(id.toInt());
        m_tags_records_map.insert(tags, records);
    }
    return result;
}

QList<HashtagPairInfo> PollPreparationDB::tag_pairs(const QVector<int> &cycle) const {
    QList<HashtagPairInfo> result;
    for (int index = 0; index < cycle.size() - 1; ++index) {
        int tag1 = m_selected_hashtags[cycle[index]]->id();
        int tag2 = m_selected_hashtags[cycle[index+1]]->id();
        QSet<int> tags;
        tags << tag1 << tag2;
        const QList<int>& records = m_tags_records_map.value(tags);
        if (!records.size()) {
            qDebug() << "Получен пустой список id записей в паре тегов " << tag1;
        }
        result.append(HashtagPairInfo(records, qMakePair(m_hashtag_info[tag1], m_hashtag_info[tag2])));
    }
    return result;
}

QString PollPreparationDB::hashtag_filters() {
    QStringList result;
    for (const HashtagFilter& filter : QueryFilters::instance().hashtags.included)
        result += filter.sign() + m_hashtag_info.value(filter.id()).name;
    for (const HashtagFilter& filter : QueryFilters::instance().hashtags.excluded)
        result += "-" + filter.sign() + m_hashtag_info.value(filter.id()).name;
    if (!result.isEmpty())
        return result.join(", ");
    return "Хэштеги не выбраны";
}
