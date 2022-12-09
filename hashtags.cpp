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
        if (e->modifiers() & Qt::ShiftModifier) {
            emit filterEvent('#', text, true);
        } else if (e->modifiers() & Qt::AltModifier) {
            emit filterEvent('#', text, false);
        } else {
            emit hashtagEvent('#', text);
        }
        break;
    case Qt::RightButton:
        if (e->modifiers() & Qt::ShiftModifier) {
            emit filterEvent('#', text, true);
        } else if (e->modifiers() & Qt::AltModifier) {
            emit filterEvent('#', text, false);
        } else {
            emit hashtagEvent('&', text);
        }
        break;
    case Qt::MiddleButton:
        if (e->modifiers() & Qt::AltModifier) {
            emit filterEvent(' ', text, false);
        } else {
            emit filterEvent(' ', text, true);
        }
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
    record_indices.clear();
    setChecked(false);
    setEnabled(true);
    show_count();
}

void HashtagButton::add_index(const QChar& sign, int index) {
    record_indices[sign].insert(index);
    ++count;
}

void HashtagButton::remove_index(const QChar& sign, int index) {
    record_indices[sign].remove(index);
    --count;
}

QSet<int> HashtagButton::indices(const QChar& sign, bool include) const {
    auto set = sign == ' '
             ? record_indices['#'] + record_indices['&']
             : record_indices[sign];
    return include ? set : all_records.subtract(set);
}

void HashtagButton::highlight(const QChar& sign, bool enable) {
    auto _font = font();
    if (sign == '#') {
        _font.setBold(enable);
    } else if (sign == '&') {
        _font.setItalic(enable);
    }
    setFont(_font);
}

void HashtagButton::highlight(bool include, bool enable) {
    auto _font = font();
    if (include) {
        _font.setUnderline(enable);
    } else {
        _font.setStrikeOut(enable);
    }
    setFont(_font);
    setChecked(enable);
    setEnabled(true);
}

QSet<int> HashtagButton::all_records;

void HashtagButton::update_on_records(int size) {
    all_records.clear();
    for (int i = 0; i < size; ++i) {
        all_records.insert(i);
    }
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
    connect(hashtags[text], SIGNAL(filterEvent(const QChar&, const QString&, bool)), this, SLOT(filter_event(const QChar&, const QString&, bool)));
    connect(hashtags[text], SIGNAL(hashtagEvent(const QChar&, const QString&)), this, SLOT(hashtag_event(const QChar&, const QString&)));
}

void MainWindow::hashtag_event(const QChar& c, const QString& text) {
    if (current_mode == IDLE) return;
    if (!filters.isEmpty()) {
        filter_event(c, text, true);
        return;
    }
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
    for (auto button : hashtags) {
        button->reset();
    }
    HashtagButton::update_on_records(records.size());
    QSet<QString> hashtags_in_config;
    for (int index = 0; index < records.size(); ++index) {
        auto i = hashtag_match(records[index].quote);
        while (i.hasNext()) {
            auto hashtag = i.peekNext().captured(1);
            auto match = i.next().captured();
            hashtags_in_config.insert(hashtag);
            if (!hashtags[hashtag]) qDebug() << "Unexpected tag: " << hashtag;
            hashtags[hashtag]->add_index(match.front(), index);
            hashtags_by_index[index].push_back(match);
        }
    }
    for (const auto& hashtag : hashtags_in_config) {
        hashtags[hashtag]->show_count();
    }
    highlight_current_hashtags(false);
    current_hashtags = hashtags_by_index[0];
    highlight_current_hashtags(true);
}

void MainWindow::recalculate_hashtags(bool increase) {
    for (const auto& tag : current_hashtags) {
        auto hashtag = tag.right(tag.size()-1);
        if (increase) {
            hashtags[hashtag]->add_index(tag.front(), pic_index);
        } else {
            hashtags[hashtag]->remove_index(tag.front(), pic_index);
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

QRegularExpressionMatchIterator MainWindow::hashtag_match(const QString& text) const {
    QRegularExpression regex("[#&]([а-яё_123]+)");
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

QString MainWindow::preprocessed(const QString& text) const {
    QString result = text;
    {
        // Setting #вопрос
        QRegularExpression question_regex("(.*)?(\\?!?\\s#)(.*)?$");
        auto i = question_regex.globalMatch(result);
        if (i.hasNext()) {
            auto match = i.peekNext();
            if (!match.captured(3).contains("вопрос")) {
                result = match.captured(1) + "?" + (match.captured(2).at(1) == '!' ? "!" : "") + " #вопрос #" + match.captured(3);
                qDebug() << result;
            }
        }
    }{
        // Replacing #программирование with #айти
        QMap<QString, QString> map = {{" #программирование", " #айти"},
                                      {" #имя", " #имена"},
                                      {" #слово", " #слова"},
                                      {" #настроение", ""},
                                      {" #доброе_утро", " #приветствие"},
                                      {" #соцсети", " #интернет"},
                                      {" #чтение", " #книги"},
                                      {" #петух", " #геи"},
                                      {" #битва", " #поединок"},
                                      {" #смешное", ""},
                                      {" #серьёзное", ""},
                                     };
        QRegularExpression it_regex("(.*)?( #программирование| #имя| #слово| "
                                    "#настроение| #доброе_утро| #соцсети| "
                                    "#чтение| #петух| #битва| #смешное| #серьёзное)(\\s.*)?$");
        auto i = it_regex.globalMatch(result);
        while (i.hasNext()) {
            auto match = i.next();
            result = match.captured(1) + map[match.captured(2)] + match.captured(3);
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
    return result;
}

void MainWindow::filter_event(const QChar& sign, const QString& text, bool include) {
    if (filters.contains(text) && (sign != filters[text].sign || include != filters[text].include)) {
        QChar c = filters[text].sign;
        QString tip = c == '#' ? "левую кнопку" : c == '&' ? "правую кнопку" : "колесико";
        ui->statusBar->showMessage("Уже активен фильтр \"" + QString(c + text).simplified() + "\". "
                                   "Нажмите " + tip + " мыши, чтобы снять действующий фильтр.");
        return;
    }
    // Disabling all buttons
    for (auto button : hashtags) {
        button->setDisabled(true);
    }
    update_filters(sign, text, include);
    // Checking and unchecking filter buttons
    hashtags[text]->setChecked(filters.contains(text));
    // Enabling necessary buttons
    if (!filters.isEmpty()) {
        // Handling the filter not used in the config
        if (filtration_results.isEmpty()) {
            hashtags[text]->setEnabled(true);
        } else show_filtering_results();
    } else exit_filtering();
    show_status();
}

void MainWindow::update_filters(const QChar& sign, const QString& text, bool include) {
    if (filters.isEmpty()) {
        ui->text->setDisabled(true);
        ui->slider->setDisabled(true);
        filters.insert(text, FilterSpecs(sign, include));
        hashtags[text]->highlight(include, true);
        apply_first_filter();
    } else if (!filters.contains(text)) {
        filters.insert(text, FilterSpecs(sign, include));
        filter(hashtags[text]->indices(sign, include));
        hashtags[text]->highlight(include, true);
    } else {
        hashtags[text]->highlight(include, false);
        filters.remove(text);
        filtration_results.clear();
        hashtags[text]->setEnabled(true);
        for (auto i = filters.begin(); i != filters.end(); ++i) {
            if (i == filters.begin()) {
                apply_first_filter();
            } else {
                filter(hashtags[i.key()]->indices(i.value().sign, i.value().include));
            }
        }
    }
}

void MainWindow::apply_first_filter() {
    auto i = filters.begin();
    for (int index : hashtags[i.key()]->indices(i.value().sign, i.value().include)) {
        filtration_results.insert(index, true);
    }
}

void MainWindow::show_filtering_results() {
    ui->slider->setValue(filtration_results.begin().key());
    ui->ok->setEnabled(filtration_results.size() > 1);
    ui->back->setDisabled(true);
    // Enabling buttons for possible non-zero result filters
    for (int index : filtration_results.keys()) {
        for (const auto& tag : hashtags_by_index[index]) {
            auto hashtag = tag.right(tag.size()-1);
            hashtags[hashtag]->setEnabled(true);
        }
    }
    // Making sure the excluding filter buttons stay enabled
    for (const auto& hashtag : filters.keys()) {
        hashtags[hashtag]->highlight(filters[hashtag].include, true);
    }
}

void MainWindow::exit_filtering() {
    for (auto button : hashtags) {
        button->setEnabled(true);
    }
    ui->ok->setEnabled(pic_index < records.size() - 1);
    ui->back->setEnabled(pic_index > 0);
    ui->text->setEnabled(true);
    ui->slider->setEnabled(true);
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
