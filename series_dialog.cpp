#include <series_dialog.h>
#include <include/database.h>
#include <QtConcurrent>

SeriesDialog::SeriesDialog(const std::set<int>& included, const QList<SeriesInfo>& titles, QWidget *parent)
    : QDialog(parent)
{
    setupUi(this);
    setWindowTitle("Выбор сериалов");

    connect(cbSorting, QOverload<int>::of(&QComboBox::currentIndexChanged), this, &SeriesDialog::sort_titles);
    connect(pbCheckAll,   &QPushButton::clicked, [this]() { check_titles(true); });
    connect(pbUncheckAll, &QPushButton::clicked, [this]() { check_titles(false); });

    for (const SeriesInfo& series : titles) {
        TitleGroup* item = new TitleGroup(series, this);
        m_titles.insert(series.id, item);
        grlTitles->addWidget(item);
        item->setChecked(included.count(series.id));
    }
    setWindowState(Qt::WindowMaximized);
}

SeriesDialog::~SeriesDialog() {
    for (auto item : m_titles)
        delete item;
}

QPair<std::set<int>, std::set<int>> SeriesDialog::results() const {
    std::set<int> checked;
    std::set<int> unchecked;
    for (auto item : m_titles)
        (item->isChecked() ? checked : unchecked).insert(item->id());
    return qMakePair(checked, unchecked);
}

void SeriesDialog::resizeEvent(QResizeEvent *event) {
    QDialog::resizeEvent(event);
    lay_titles(event);
}

void SeriesDialog::check_titles(bool enable) {
    for (auto item : m_titles)
        item->setChecked(enable);
}

void SeriesDialog::sort_titles() {
    int title_width = 192;
    int columns = std::max(1, (saTitles->width() - 100) / title_width);
    m_current_title_columns = columns;
    QLayoutItem* child;
    while ((child = grlTitles->takeAt(0))) {
        // Clearing items from the grid
//        child->widget()->hide();
    }
    using Iterator = QMap<int, TitleGroup*>::const_iterator;
    QVector<Iterator> iters;
    iters.reserve(m_titles.size());
    for (auto it = m_titles.cbegin(); it != m_titles.cend(); ++it) {
        iters.push_back(it);
    }
    static auto checked_title_sort = [](const Iterator& it1, const Iterator& it2){
        auto title1 = it1.value();
        auto title2 = it2.value();
        if (title1->isChecked() != title2->isChecked()) {
            return title1->isChecked() > title2->isChecked();
        }
        return it1.key() < it2.key();
    };
    static auto title_size_sort = [this](const Iterator& it1, const Iterator& it2){
        auto title1 = it1.value();
        auto title2 = it2.value();
        return title1->size() > title2->size();
    };
    if (cbSorting->currentIndex() == 1) {
        std::sort(iters.begin(), iters.end(), checked_title_sort);
    } else if (cbSorting->currentIndex() == 2) {
        std::sort(iters.begin(), iters.end(), title_size_sort);
    }
    int i = 0;
    for (const auto iter : iters) {
        auto title_item = m_titles[iter.key()];
        grlTitles->addWidget(title_item, i/columns, i%columns, Qt::AlignCenter);
        title_item->show();
        ++i;
    }
}

void SeriesDialog::lay_titles(QResizeEvent *event) {
    int title_width = 192;
    int columns = (saTitles->width() - 100) / title_width;
    if (columns == m_current_title_columns) return;
    m_current_title_columns = columns;
    QVector<QLayoutItem*> items;
    items.resize(grlTitles->children().size());
    QLayoutItem* child;
    while ((child = grlTitles->takeAt(0))) {
        items.push_back(child);
    }
    int i = 0;
    for (const auto item : m_titles) {
        grlTitles->addWidget(item, i / columns, i % columns);
        ++i;
    }
}
