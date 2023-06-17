#ifndef RECORD_ITEMS_H
#define RECORD_ITEMS_H
#include <QHBoxLayout>
#include <QJsonArray>
#include <QJsonObject>
#include <QLabel>
#include <QCheckBox>
#include <QMouseEvent>
#include <QtConcurrent>

struct Record {
    Record(const QString& quote = QString()) : quote(quote) {}
    QString quote;
    QStringList pics;
    QList<int> ids;
    QStringList links;
    bool is_public;
    QJsonObject to_json() const;
};

class RecordItem : public QWidget
{
    Q_OBJECT
public:
    RecordItem(const Record&, int, const QString&);
    RecordItem(const QString&, int);
    ~RecordItem() override {}
    void set_gallery_view();
    void set_list_view();
    void mouseDoubleClickEvent(QMouseEvent*) override;
    void update_text(const QString&);
signals:
    void selected(int);
private:
    int index;
    QLabel image;
    QLabel text;
    QLabel number;
    QCheckBox box;
    QGridLayout layout;
    void load_thumbmnail(const QString&);
};

#endif // RECORD_ITEMS_H
