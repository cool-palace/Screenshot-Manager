#include "locations.h"
#include "posting_progress_dialog.h"

PostingProgressDialog::PostingProgressDialog(QWidget *parent) : QDialog(parent) {
    setWindowTitle("Публикация записей");
    resize(500, 300);

    m_progress = new QProgressBar(this);
    m_progress->setRange(0, m_records.size());
    m_progress->setValue(0);

    m_status = new QLabel("Ожидание начала...", this);
    m_log = new QTextEdit(this);
    m_log->setReadOnly(true);

    auto layout = new QVBoxLayout(this);
    layout->addWidget(m_progress);
    layout->addWidget(m_status);
    layout->addWidget(m_log);

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
    m_status->setText("Начинаем публикацию...");
    if (!m_hashtags.size())
        post_next();
    else
        get_poll();
}

void PostingProgressDialog::handle_success(int index, int date) {
    m_log->append(QString("✅ Запись %1 опубликована (дата: %2)").arg(index).arg(QDateTime::fromSecsSinceEpoch(date).toString(Qt::ISODate)));
    ++m_success_count;
    m_progress->setValue(++m_current);
    QTimer::singleShot(200, this, &PostingProgressDialog::post_next);
}

void PostingProgressDialog::handle_failure(int index, const QString &error) {
    m_log->append(QString("❌ Ошибка публикации %1: %2").arg(index).arg(error));
    ++m_fail_count;
    m_progress->setValue(++m_current);
    QTimer::singleShot(200, this, &PostingProgressDialog::post_next);
}

void PostingProgressDialog::post_next() {
    if (m_current >= m_records.size()) {
        m_status->setText(QString("Готово: %1 успешно, %2 с ошибкой").arg(m_success_count).arg(m_fail_count));
        update_record_logs();
        return;
    }

    const RecordPreviewBase* record = m_records[m_current];
    m_status->setText(QString("Публикация записи %1 из %2...").arg(m_current+1).arg(m_records.size()));
    record->post();
}

void PostingProgressDialog::handle_poll_success() {
    m_log->append(QString("✅ Опрос опубликован (дата: %1)").arg(m_poll_times.first.toString("dd-MM-yyyy HH:mm")));
    update_poll_logs();
    QTimer::singleShot(200, this, &PostingProgressDialog::post_next);
}

void PostingProgressDialog::handle_poll_failure(const QString &error) {
    m_log->append(QString("❌ Ошибка публикации опроса: %1").arg(error));
    QTimer::singleShot(200, this, &PostingProgressDialog::post_next);
}

void PostingProgressDialog::get_poll() {
    VK_Manager::instance().get_poll(poll_options(), m_poll_times.second.toSecsSinceEpoch());
}

void PostingProgressDialog::post_poll(int id) {
    m_log->append(QString("✅ Опрос создан, id = %1").arg(id));
    VK_Manager::instance().post(poll_message(), id, m_poll_times.first.toSecsSinceEpoch());
}

void PostingProgressDialog::update_record_logs() {
    QMap<int, QDateTime> logs;
    for (const RecordPreviewBase* record : m_records)
        logs.insert(record->logs_data());

    // Обновление логов в базе данных
    int query_result = Database::instance().update_record_logs(logs);
    if (query_result == logs.size())
        m_log->append(QString("✅ Логи публикаций в базе данных обновлены."));
    else if (query_result > 0)
        m_log->append(QString("❌ Не удалось обновить %1 записей в логах публикаций в БД.").arg(logs.size() - query_result));
    else
        m_log->append(QString("❌ Не удалось провести транзакцию в логах публикаций в БД."));

    // Обновление текстовых логов
    QString logs_filepath = Locations::instance()[LOGS_FILE];
    QJsonObject logs_json = json_object(logs_filepath);
    QFile file(logs_filepath);
    for (auto it = logs.cbegin(); it != logs.cend(); ++it) {
        logs_json[QString().setNum(it.key())] = it.value().toSecsSinceEpoch();
    }
    if (save_json(logs_json, file))
        m_log->append(QString("✅ Текстовые логи публикаций обновлены."));
    else
        m_log->append(QString("❌ Не удалось обновить текстовые логи публикаций."));
}

void PostingProgressDialog::update_poll_logs() {
    if (m_hashtags.empty()) return;

    QStringList tags;
    tags.reserve(m_hashtags.size());
    for (const HashtagPreviewDB* hashtag : m_hashtags)
        tags.append(hashtag->name());

    // Обновление логов в базе данных
    int query_result = Database::instance().update_poll_logs(tags, m_poll_times.second);
    if (query_result == tags.size())
        m_log->append(QString("✅ Логи опросов в базе данных обновлены."));
    else if (query_result > 0)
        m_log->append(QString("❌ Не удалось обновить %1 записей в логах опросов в БД.").arg(tags.size() - query_result));
    else
        m_log->append(QString("❌ Не удалось провести транзакцию в логах опросов в БД."));

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
        m_log->append(QString("✅ Текстовые логи опросов обновлены."));
    else
        m_log->append(QString("❌ Не удалось обновить текстовые логи опросов."));
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
