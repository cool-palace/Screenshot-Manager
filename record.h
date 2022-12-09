#ifndef RECORD_ITEMS_H
#define RECORD_ITEMS_H
#include <QHBoxLayout>
#include <QJsonArray>
#include <QJsonObject>
#include <QLabel>
#include <QCheckBox>
#include <QMouseEvent>

struct Record {
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
    ~RecordItem() override;
    void set_gallery_view();
    void set_list_view();
    void mouseDoubleClickEvent(QMouseEvent*) override;
signals:
    void selected(int);
private:
    int index;
    QLabel* image;
    QLabel* text;
    QCheckBox* box;
    QGridLayout *layout;
};

#endif // RECORD_ITEMS_H
