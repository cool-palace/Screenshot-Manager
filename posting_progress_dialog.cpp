#include "locations.h"
#include "posting_progress_dialog.h"

PostingProgressDialog::PostingProgressDialog(QWidget *parent) : QDialog(parent) {
    setWindowTitle("Публикация записей");
    resize(500, 300);

    pbProgress = new QProgressBar(this);
    pbProgress->setRange(0, m_records.size());
    pbProgress->setValue(0);

    lblStatus = new QLabel("Ожидание начала...", this);
    teLog = new QTextEdit(this);
    teLog->setReadOnly(true);

    auto layout = new QVBoxLayout(this);
    layout->addWidget(pbProgress);
    layout->addWidget(lblStatus);
    layout->addWidget(teLog);

    connect(&VK_Manager::instance(), &VK_Manager::posted_successfully, this, &PostingProgressDialog::handle_success);
    connect(&VK_Manager::instance(), &VK_Manager::post_failed, this, &PostingProgressDialog::handle_failure);
}

PostingProgressDialog::PostingProgressDialog(const QList<RecordPreviewDB*> &records, QWidget *parent)
    : PostingProgressDialog(parent)
{
    for (RecordPreviewDB* record : records)
        m_records.append(static_cast<RecordPreviewBase*>(record));
}

PostingProgressDialog::PostingProgressDialog(const QList<HashtagPreviewDB*>& hashtags, QPair<QDateTime, QDateTime>&& poll_time, const QList<RecordPollPreview *> &records, QWidget *parent)
    : PostingProgressDialog(parent)
{
    for (RecordPollPreview* record : records)
        m_records.append(static_cast<RecordPreviewBase*>(record));

    m_poll_times = std::move(poll_time);
    m_hashtags = hashtags;
    std::sort(m_hashtags.begin(), m_hashtags.end(), [](HashtagPreviewDB* a, HashtagPreviewDB* b) {
        return a->id() < b->id();
    });
    connect(&VK_Manager::instance(), &VK_Manager::poll_ready, this, &PostingProgressDialog::post_poll);
    connect(&VK_Manager::instance(), &VK_Manager::poll_posted_successfully, this, &PostingProgressDialog::handle_poll_success);
    connect(&VK_Manager::instance(), &VK_Manager::poll_post_failed, this, &PostingProgressDialog::handle_poll_failure);
}

void PostingProgressDialog::start_posting() {
    m_current = 0;
    m_success_count = 0;
    m_fail_count = 0;
    lblStatus->setText("Начинаем публикацию...");
    if (!m_hashtags.size())
        post_next();
    else
        get_poll();
}

void PostingProgressDialog::handle_success(int index, int date) {
    teLog->append(QString("✅ Запись %1 опубликована (дата: %2)").arg(index).arg(QDateTime::fromSecsSinceEpoch(date).toString(Qt::ISODate)));
    ++m_success_count;
    pbProgress->setValue(++m_current);
    QTimer::singleShot(200, this, &PostingProgressDialog::post_next);
}

void PostingProgressDialog::handle_failure(int index, const QString &error) {
    teLog->append(QString("❌ Ошибка публикации %1: %2").arg(index).arg(error));
    ++m_fail_count;
    pbProgress->setValue(++m_current);
    QTimer::singleShot(200, this, &PostingProgressDialog::post_next);
}

void PostingProgressDialog::post_next() {
    if (m_current >= m_records.size()) {
        lblStatus->setText(QString("Готово: %1 успешно, %2 с ошибкой").arg(m_success_count).arg(m_fail_count));
        update_record_logs();
        return;
    }

    const RecordPreviewBase* record = m_records[m_current];
    lblStatus->setText(QString("Публикация записи %1 из %2...").arg(m_current+1).arg(m_records.size()));
    record->post();
}

void PostingProgressDialog::handle_poll_success() {
    teLog->append(QString("✅ Опрос опубликован (дата: %1)").arg(m_poll_times.first.toString("dd-MM-yyyy HH:mm")));
    update_poll_logs();
    update_hashtags();
    QTimer::singleShot(200, this, &PostingProgressDialog::post_next);
}

void PostingProgressDialog::handle_poll_failure(const QString &error) {
    teLog->append(QString("❌ Ошибка публикации опроса: %1").arg(error));
    QTimer::singleShot(200, this, &PostingProgressDialog::post_next);
}

void PostingProgressDialog::get_poll() {
    VK_Manager::instance().get_poll(poll_options(), m_poll_times.second.toSecsSinceEpoch());
}

void PostingProgressDialog::post_poll(int id) {
    teLog->append(QString("✅ Опрос создан, id = %1").arg(id));
    VK_Manager::instance().post(poll_message(), id, m_poll_times.first.toSecsSinceEpoch());
}

void PostingProgressDialog::update_record_logs() {
    QMap<int, QDateTime> logs;
    for (const RecordPreviewBase* record : m_records)
        logs.insert(record->logs_data());

    // Обновление логов в базе данных
    int query_result = Database::instance().update_record_logs(logs);
    if (query_result == logs.size())
        teLog->append(QString("✅ Логи публикаций в базе данных обновлены."));
    else if (query_result > 0)
        teLog->append(QString("❌ Не удалось обновить %1 записей в логах публикаций в БД.").arg(logs.size() - query_result));
    else
        teLog->append(QString("❌ Не удалось провести транзакцию в логах публикаций в БД."));

    // Обновление текстовых логов
    QString logs_filepath = Locations::instance()[LOGS_FILE];
    QJsonObject logs_json = json_object(logs_filepath);
    QFile file(logs_filepath);
    for (auto it = logs.cbegin(); it != logs.cend(); ++it) {
        logs_json[QString().setNum(it.key())] = it.value().toSecsSinceEpoch();
    }
    if (save_json(logs_json, file))
        teLog->append(QString("✅ Текстовые логи публикаций обновлены."));
    else
        teLog->append(QString("❌ Не удалось обновить текстовые логи публикаций."));
}

void PostingProgressDialog::update_poll_logs() {
    if (m_hashtags.empty()) return;

    QStringList tags;
    tags.reserve(m_hashtags.size());
    for (const HashtagPreviewDB* hashtag : m_hashtags)
        tags.append(hashtag->name());

    // Обновление логов в базе данных
    int query_result = Database::instance().update_poll_logs(tags, m_poll_times.first);
    if (query_result == tags.size())
        teLog->append(QString("✅ Логи опросов в базе данных обновлены."));
    else if (query_result > 0)
        teLog->append(QString("❌ Не удалось обновить %1 записей в логах опросов в БД.").arg(tags.size() - query_result));
    else
        teLog->append(QString("❌ Не удалось провести транзакцию в логах опросов в БД."));

    // Обновление текстовых логов
    QString logs_filepath = Locations::instance()[POLL_LOGS];
    QJsonObject logs_json = json_object(logs_filepath);
    QFile file(logs_filepath);
    qint64 time = m_poll_times.first.toSecsSinceEpoch();
    for (int i = 0; i < m_hashtags.size(); ++i) {
        QString tag = m_hashtags[i]->name();
        logs_json[tag] = time;
    }
    if (save_json(logs_json, file))
        teLog->append(QString("✅ Текстовые логи опросов обновлены."));
    else
        teLog->append(QString("❌ Не удалось обновить текстовые логи опросов."));
}

void PostingProgressDialog::update_hashtags() {
    if (m_hashtags.empty()) return;

    QMap<int, QString> modified_descriptions;
    for (const HashtagPreviewDB* hashtag : m_hashtags)
        if (hashtag->description_edited())
            modified_descriptions.insert(hashtag->id(), hashtag->current_description());

    if (modified_descriptions.isEmpty()) return;

    // Обновление описаний хэштегов в базе данных
    int query_result = Database::instance().update_hashtag_descriptions(modified_descriptions);
    if (query_result == modified_descriptions.size())
        teLog->append(QString("✅ Описания хэштегов в базе данных обновлены."));
    else if (query_result > 0)
        teLog->append(QString("❌ Не удалось обновить %1 описаний хэштегов в БД.").arg(modified_descriptions.size() - query_result));
    else
        teLog->append(QString("❌ Не удалось провести транзакцию в таблице хэштегов в БД."));

    // Обновление текстовых описаний хэштегов
    QString filepath = Locations::instance()[HASHTAGS];
    QJsonObject hashtags_json = json_object(filepath);
    QFile file(filepath);
    for (const HashtagPreviewDB* hashtag : m_hashtags) {
        QString tag = hashtag->name();
        hashtags_json[tag].toObject()["description"] = hashtag->current_description();
    }
    if (save_json(hashtags_json, file))
        teLog->append(QString("✅ Текстовые описания хэштегов обновлены."));
    else
        teLog->append(QString("❌ Не удалось обновить текстовые описания хэштегов."));
}

QString PostingProgressDialog::poll_options() const {
    QStringList options;
    options.reserve(m_hashtags.size());
    for (const HashtagPreviewDB* tag : m_hashtags)
        options += tag->option();
    return QString("[%1]").arg(options.join(','));
}

QString PostingProgressDialog::poll_message() const {
    QString text;
    for (int i = 1; i <= m_hashtags.size(); ++i)
        text.append(QString("%1. %2\n").arg(i).arg(m_hashtags[i-1]->message()));
    return text;
}
