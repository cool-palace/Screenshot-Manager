#include "title_group.h"
#include <include/common.h>

const QSize TitleGroup::pic_size = QSize(160, 90);

TitleGroup::TitleGroup(const QString &name, const QString &filepath, int size, QWidget *parent)
    : QGroupBox(parent), m_name(name), m_filepath(filepath), m_size(size)
{
    setTitle(elide_by_word_limit(m_name, 20));

    auto pic = QImage(filepath);
    if (pic.isNull()) pic = QImage(filepath.chopped(3) + "jpg");
    lblImage->setPixmap(QPixmap::fromImage(pic.scaled(pic_size, Qt::KeepAspectRatio)));

    lblSize->setText(QString("%1 %2").arg(m_size).arg(inflect(m_size, "кадров")));
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
