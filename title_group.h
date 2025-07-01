#ifndef TITLE_GROUP_H
#define TITLE_GROUP_H
#include <QGroupBox>
#include <QRegularExpression>
#include <ui_title_group.h>

class TitleGroup : public QGroupBox, public Ui_TitleGroup
{
    Q_OBJECT
public:
    explicit TitleGroup(const QString& name, const QString& pic, int size, QWidget *parent = nullptr);
    const QString name() const { return m_name; }
    int size() const { return m_size; }

private:
    static QString elide_by_word_limit(const QString& input, int maxLength = 20);

private:
    static const QSize pic_size;
    QString m_name;
    QString m_filepath;
    int m_size;
};

#endif // TITLE_GROUP_H
