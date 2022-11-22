#include "mainwindow.h"
#include "ui_mainwindow.h"
#include <QDebug>

HashtagButton::HashtagButton(const QString& text, MainWindow* window) :
    QPushButton(text),
    parent(window),
    text(text)
{
    setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Expanding);
    setToolTip(text);
}

void HashtagButton::mousePressEvent(QMouseEvent * e) {
    if (e->button() == Qt::LeftButton)
        hashtagEvent('#');
    else if (e->button() == Qt::RightButton)
        hashtagEvent('&');
}

void HashtagButton::hashtagEvent(QChar c) {
    if (parent->is_idle()) return;
    QRegularExpression regex(QString("(.*)?") + c + text + "(\\s.*)?$");
    auto i = regex.globalMatch(parent->text());
    bool hashtag_is_in_text = i.hasNext();
    if (!hashtag_is_in_text) {
        parent->set_text(parent->text() + " " + c + text);
    } else {
        auto match = i.peekNext();
        parent->set_text(match.captured(1).chopped(1) + match.captured(2));
    }
    highlight(c, !hashtag_is_in_text);
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
    hashtags.insert(text, new HashtagButton(text, this));
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
    for (auto hashtag : hashtags_count.keys()) {
        auto button = hashtags.value(hashtag);
        if (button) {
            button->setText(button->toolTip());
            button->setFlat(false);
        }
    }
    hashtags_count.clear();
    for (auto record : records) {
        auto i = hashtag_match(record.quote);
        while (i.hasNext()) {
            auto match = i.next().captured(1);
            if (hashtags_count.contains(match)) {
                ++hashtags_count[match];
            } else {
                hashtags_count[match] = 1;
            }
        }
    }
    for (auto hashtag : hashtags_count.keys()) {
        auto button = hashtags.value(hashtag);
        if (button) {
            button->setText(button->toolTip() + ' ' + QString().setNum(hashtags_count[hashtag]));
            button->setFlat(true);
        }
    }
}

void MainWindow::recalculate_hashtags(bool increase) {
    for (const auto& hashtag_pair : current_hashtags) {
        auto hashtag = hashtag_pair.second;
        size_t count = increase ? ++hashtags_count[hashtag] : --hashtags_count[hashtag];
        auto button = hashtags.value(hashtag);
        if (button) {
            button->setText(button->toolTip() + ' ' + QString().setNum(count));
            button->setFlat(count);
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
        current_hashtags.push_back(qMakePair(i.peekNext().captured()[0], i.peekNext().captured(1)));
        i.next();
    }
    highlight_current_hashtags(true);
}

QRegularExpressionMatchIterator MainWindow::hashtag_match(const QString& text) {
    QRegularExpression regex("[#&]([а-яё_]+)");
    return regex.globalMatch(text);
}

void MainWindow::highlight_current_hashtags(bool enable) {
    for (auto hashtag_pair : current_hashtags) {
        auto hashtag = hashtag_pair.second;
        auto button = hashtags.value(hashtag);
        if (!button) {
            qDebug() << "Unexpected tag: " << hashtag;
            return;
        }
        button->highlight(hashtag_pair.first, enable);
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
