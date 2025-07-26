#include "posting_progress_dialog.h"

PostingProgressDialog::PostingProgressDialog(QWidget *parent) : QDialog(parent) {
    setWindowTitle("Публикация записей");
    resize(500, 300);

    m_progress = new QProgressBar(this);
    m_progress->setRange(0, m_records.size());

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
        m_status->setText(QString("Готово: %1 успешных, %2 с ошибкой").arg(m_success_count).arg(m_fail_count));
        return;
    }

    const RecordPreviewBase* record = m_records[m_current];
    m_status->setText(QString("Публикация записи %1 из %2...").arg(m_current+1).arg(m_records.size()));
    record->post();
}

void PostingProgressDialog::handle_poll_success() {
    m_log->append(QString("✅ Опрос опубликован (дата: %1)").arg(m_poll_times.first.toString(Qt::ISODate)));
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
