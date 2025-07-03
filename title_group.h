#ifndef TITLE_GROUP_H
#define TITLE_GROUP_H
#include <include/series_info.h>
#include <QGroupBox>
#include <QRegularExpression>
#include <ui_title_group.h>

class TitleGroup : public QGroupBox, public Ui_TitleGroup
{
    Q_OBJECT
public:
    TitleGroup(const SeriesInfo& series, QWidget *parent = nullptr);
    const QString name() const { return m_name; }
    int id() const {return m_id; }
    int size() const { return m_size; }
    void set_text();
    void load_thumbmnail();

private:
    static QString elide_by_word_limit(const QString& input, int maxLength = 20);

private:
    int m_id;
    QString m_name;
    QString m_filepath;
    int m_size;
};

#endif // TITLE_GROUP_H
