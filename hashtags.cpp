#include "mainwindow.h"
#include "ui_mainwindow.h"
#include <QDebug>

HashtagButton::HashtagButton(const QString& text) :
    QPushButton(text),
    text(text)
{
    setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Expanding);
    setToolTip(text);
    setCheckable(true);
}

void HashtagButton::mousePressEvent(QMouseEvent * e) {
    switch (e->button()) {
    case Qt::LeftButton:
        emit hashtagEvent('#', text);
        break;
    case Qt::RightButton:
        emit hashtagEvent('&', text);
        break;
    case Qt::MiddleButton:
        setChecked(!isChecked());
        emit filterEvent(text);
        break;
    default:
        break;
    }
}

void HashtagButton::show_count() {
    setText(count ? text + ' ' + QString().setNum(count) : text);
    setFlat(count);
}

void HashtagButton::reset() {
    count = 0;
    show_count();
}

void HashtagButton::add_index(int index) {
    record_indices.insert(index);
    ++count;
}

void HashtagButton::remove_index(int index) {
    record_indices.remove(index);
    --count;
}

void HashtagButton::highlight(QChar c, bool enable) {
    auto _font = font();
    if (c == '#') {
        _font.setBold(enable);
    } else if (c == '&') {
        _font.setItalic(enable);
    }
    setFont(_font);
}

void MainWindow::get_hashtags() {
    QFile file("hashtags.txt");
    if (!file.open(QIODevice::ReadOnly)) {
        ui->statusBar->showMessage("Не удалось открыть файл с хэштегами.");
    }
    QTextStream in(&file);
    in.setCodec("UTF-8");
    while (!in.atEnd()) {
        QString tag = in.readLine();
        if (tag[0] == '#') {
            tag.remove(0,1);
        }
        create_hashtag_button(tag);
    }
    file.close();
    update_hashtag_grid();
}

void MainWindow::create_hashtag_button(const QString& text) {
    hashtags.insert(text, new HashtagButton(text));
    connect(hashtags[text], SIGNAL(filterEvent(const QString&)), this, SLOT(filter_update(const QString&)));
    connect(hashtags[text], SIGNAL(hashtagEvent(QChar, const QString&)), this, SLOT(hashtag_event(QChar, const QString&)));
}

void MainWindow::hashtag_event(QChar c, const QString& text) {
    if (current_mode == IDLE) return;
    QRegularExpression regex(QString("(.*)?") + c + text + "(\\s.*)?$");
    auto i = regex.globalMatch(ui->text->toPlainText());
    bool hashtag_is_in_text = i.hasNext();
    if (!hashtag_is_in_text) {
        ui->text->setText(ui->text->toPlainText() + " " + c + text);
    } else {
        auto match = i.peekNext();
        ui->text->setText(match.captured(1).chopped(1) + match.captured(2));
    }
    hashtags[text]->highlight(c, !hashtag_is_in_text);
}

void MainWindow::update_hashtag_grid() {
    while (ui->tag_grid->takeAt(0) != nullptr) {
        // Clearing buttons from the grid
    }
    int i = 0;
    for (auto button : hashtags) {
        ui->tag_grid->addWidget(button, i / 10, i % 10);
        ++i;
    }
    ui->tag_grid->addWidget(add_tag_button, i / 10, i % 10);
}

void MainWindow::load_hashtag_info() {
    for (const auto& hashtag : hashtags_in_config) {
        auto button = hashtags.value(hashtag);
        if (button) {
            button->reset();
        }
    }
    hashtags_in_config.clear();
    for (int index = 0; index < records.size(); ++index) {
        auto i = hashtag_match(records[index].quote);
        while (i.hasNext()) {
            auto hashtag = i.peekNext().captured(1);
            auto match = i.next().captured();
            hashtags_in_config.insert(hashtag);
            if (!hashtags[hashtag]) qDebug() << "Unexpected tag: " << hashtag;
            hashtags[hashtag]->add_index(index);
            hashtags_by_index[index].push_back(match);
        }
    }
    for (const auto& hashtag : hashtags_in_config) {
        auto button = hashtags.value(hashtag);
        if (button) {
            button->show_count();
        }
    }
    highlight_current_hashtags(false);
    current_hashtags = hashtags_by_index[0];
    highlight_current_hashtags(true);
}

void MainWindow::recalculate_hashtags(bool increase) {
    for (const auto& tag : current_hashtags) {
        auto hashtag = tag.right(tag.size()-1);
        if (increase) {
            hashtags[hashtag]->add_index(pic_index);
        } else {
            hashtags[hashtag]->remove_index(pic_index);
        }
        auto button = hashtags.value(hashtag);
        if (button) {
            button->show_count();
        }
    }
}

void MainWindow::update_hashtag_info() {
    recalculate_hashtags(false);
    update_current_hashtags();
    recalculate_hashtags(true);
}

void MainWindow::update_current_hashtags() {
    highlight_current_hashtags(false);
    current_hashtags.clear();
    auto i = hashtag_match(records[pic_index].quote);
    while (i.hasNext()) {
        current_hashtags.push_back(i.next().captured());
    }
    hashtags_by_index[pic_index] = current_hashtags;
    highlight_current_hashtags(true);
}

QRegularExpressionMatchIterator MainWindow::hashtag_match(const QString& text) {
    QRegularExpression regex("[#&]([а-яё_]+)");
    return regex.globalMatch(text);
}

void MainWindow::highlight_current_hashtags(bool enable) {
    for (auto tag : current_hashtags) {
        auto hashtag = tag.right(tag.size()-1);
        auto button = hashtags.value(hashtag);
        if (!button) {
            qDebug() << "Unexpected tag: " << hashtag;
            return;
        }
        button->highlight(tag.front(), enable);
    }
}

QString MainWindow::preprocessed(const QString& text) {
    QString result = text;
    {
        // Replacing #программирование with #айти
        QRegularExpression it_regex("(.*\\s)?#программирование(\\s.*)?$");
        auto i = it_regex.globalMatch(text);
        if (i.hasNext()) {
            auto match = i.peekNext();
            result = match.captured(1) + "#айти" + match.captured(2);
            qDebug() << result;
        }
    }{
        // Replacing #реакция+#hashtag with &hashtag
        QRegularExpression reaction_regex("(.*\\s)?#реакция(\\s.*)?$");
        auto i = reaction_regex.globalMatch(result);
        QSet<QString> exceptions({"#обман", "#глупость", "#игнор"});
        if (i.hasNext()) {
            auto match = i.peekNext();
            auto reactive_tags = match.captured(2).split(' ', QString::SkipEmptyParts);
            for (auto& tag : reactive_tags) {
                if (tag[0] == '#' && !exceptions.contains(tag)) {
                    tag.replace(0, 1, "&");
                }
            }
            result = match.captured(1) + reactive_tags.join(" ");
            qDebug() << result;
        }
    }
    return text;
}

void MainWindow::filter_update(const QString & text) {
    // Disabling all buttons
    for (auto button : hashtags) {
        button->setDisabled(true);
    }
    // Updating filters
    if (filters.isEmpty()) {
        filters.insert(text);
        for (int index : hashtags[text]->indices()) {
            filtration_results.insert(index, true);
        }
    } else if (!filters.contains(text)) {
        filters.insert(text);
        filter(hashtags[text]->indices());
    } else {
        filters.remove(text);
        filtration_results.clear();
        QSetIterator<QString> i(filters);
        while (i.hasNext()) {
            if (!i.hasPrevious()) {
                for (int index : hashtags[i.next()]->indices()) {
                    filtration_results.insert(index, true);
                }
            } else {
                filter(hashtags[i.next()]->indices());
            }
        }
    }
    qDebug() << filtration_results.keys();
    // Enabling necessary buttons
    if (!filters.isEmpty()) {
        // Handling the filter not used in the config
        if (filtration_results.isEmpty()) {
            hashtags[text]->setEnabled(true);
        }
        // Enabling buttons for possible non-zero result filters
        for (int index : filtration_results.keys()) {
            for (const auto& tag : hashtags_by_index[index]) {
                auto hashtag = tag.right(tag.size()-1);
                hashtags[hashtag]->setEnabled(true);
            }
        }
    } else for (auto button : hashtags) {
        button->setEnabled(true);
    }
}

void MainWindow::filter(const QSet<int>& second) {
    if (filtration_results.isEmpty()) return;
    QMap<int, bool> result;
    auto keys = QSet<int>::fromList(filtration_results.keys()).intersect(second);
    for (const auto& key : keys) {
        result.insert(key,true);
    }
    filtration_results = result;
}