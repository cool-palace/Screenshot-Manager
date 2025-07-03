#include "title_group.h"
#include <include/common.h>
#include <QtConcurrent>
#include "locations.h"

TitleGroup::TitleGroup(const SeriesInfo& series, QWidget *parent)
    : QGroupBox(parent), m_id(series.id), m_name(series.name), m_filepath(Locations::instance()[SCREENSHOTS] + series.filepath), m_size(series.size)
{
    setupUi(this);
    setTitle(elide_by_word_limit(m_name, 24));
    QtConcurrent::run(this, &TitleGroup::load_thumbmnail);
    set_text();
}

void TitleGroup::set_text() {
    lblSize->setText(QString("%1 %2").arg(m_size).arg(inflect(m_size, "записей")));
}

void TitleGroup::load_thumbmnail() {
    static const QSize pic_size = QSize(192, 108);
    auto pic = QImage(m_filepath);
    if (pic.isNull())
        pic = QImage(m_filepath.chopped(3) + "jpg");
    if (!pic.isNull())
        lblImage->setPixmap(QPixmap::fromImage(pic.scaled(pic_size, Qt::KeepAspectRatio)));
}

QString TitleGroup::elide_by_word_limit(const QString &input, int max_length) {
    if (input.length() <= max_length)
        return input;

    QStringList words = input.split(QRegularExpression("\\s+"), Qt::SkipEmptyParts);
    QString result;
    for (const QString& word : words) {
        if (result.length() + word.length() + 1 > max_length)
            break;

        if (!result.isEmpty())
            result += ' ';
        result += word;
    }

    return result.trimmed() + "…";
}
