#ifndef RECORD_ITEMS_H
#define RECORD_ITEMS_H
#include <QHBoxLayout>
#include <QJsonArray>
#include <QJsonObject>
#include <QLabel>
#include <QCheckBox>
#include <QMouseEvent>
#include <QtConcurrent>
#include "vk_manager.h"

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

class RecordFrame : public QLabel
{
    Q_OBJECT
public:

private:
    static VK_Manager* manager;
};


class RecordPreview : public RecordItem
{
    Q_OBJECT
public:
    RecordPreview(const Record&, int);
    ~RecordPreview() override {}
//    void mouseDoubleClickEvent(QMouseEvent*) override;
private:
    Record record;
};

#endif // RECORD_ITEMS_H
