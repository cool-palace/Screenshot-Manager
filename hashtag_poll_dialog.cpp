#include "hashtag_poll_dialog.h"

int HashtagPollDialog::m_button_width = 130;

HashtagPollDialog::HashtagPollDialog(const QMap<int, HashtagInfo> &hashtags, const QList<HashtagPreviewDB*>& selected, QWidget *parent)
    : QDialog(parent)
{
    setupUi(this);
    setWindowTitle("Выбор хэштегов");
    lblMaxRank->hide();
    lblMinRank->hide();
    lblSorting->hide();
    sbMinRank->hide();
    sbMaxRank->hide();
    cbSorting->hide();

    QSet<int> selected_ids;
    for (const HashtagPreviewDB* hashtag : selected)
        selected_ids.insert(hashtag->id());

    for (const HashtagInfo& hashtag : hashtags) {
        HashtagPollButton* button = new HashtagPollButton(hashtag, this);
        m_hashtags.insert(hashtag.id, button);
        grlHashtags->addWidget(button);
        // Включаем только те кнопки, которые ещё не выбраны
        button->setEnabled(!selected_ids.contains(hashtag.id));
        // Подключаем сигнал
        connect(button, &HashtagPollButton::selected, [this](const HashtagInfo& hashtag) {
            m_result = std::move(hashtag);
            accept();
        });
    }
    setWindowState(Qt::WindowMaximized);
}

void HashtagPollDialog::resizeEvent(QResizeEvent *event) {
    QDialog::resizeEvent(event);
    lay_hashtags(event);
}

void HashtagPollDialog::sort_hashtags() {

}

void HashtagPollDialog::lay_hashtags(QResizeEvent *event) {
    int columns = std::max(1, (saHashtags->width() - 100) / m_button_width);
    if (columns == m_current_tag_columns) return;
    m_current_tag_columns = columns;
    QVector<QLayoutItem*> items;
    items.resize(grlHashtags->children().size());
    QLayoutItem* child;
    while ((child = grlHashtags->takeAt(0))) {
        items.push_back(child);
    }
    int i = 0;
    for (const auto item : items) {
        grlHashtags->addItem(item, i / columns, i % columns);
        ++i;
    }
}
