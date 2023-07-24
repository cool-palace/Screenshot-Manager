#ifndef RECORD_ITEMS_H
#define RECORD_ITEMS_H
#include <QHBoxLayout>
#include <QJsonArray>
#include <QJsonObject>
#include <QLabel>
#include <QCheckBox>
#include <QMouseEvent>
#include <QtConcurrent>
#include <QPushButton>
#include <QInputDialog>
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

class RecordBase : public QWidget
{
    Q_OBJECT
public:
    RecordBase(const Record&, int);
    ~RecordBase() override {}
    void mouseDoubleClickEvent(QMouseEvent*) override;
    void update_text(const QString&);
    virtual void set_gallery_view() = 0;
    virtual void set_list_view() = 0;
signals:
    void selected(int);
protected:
    int index;
    QString pic;
    QLabel image;
    QLabel text;
    QLabel number;
    QGridLayout layout;
};

class RecordItem : public RecordBase
{
    Q_OBJECT
public:
    RecordItem(const Record&, int, const QString&);
    RecordItem(const QString&, int);
    ~RecordItem() override {}
    void set_gallery_view() override;
    void set_list_view() override;
//    void mouseDoubleClickEvent(QMouseEvent*) override;
//    void update_text(const QString&);
//signals:
//    void selected(int);
private:
    int index;
    QCheckBox box;
    void load_thumbmnail(const QString&);
};

class RecordFrame : public QLabel
{
    Q_OBJECT
public:
    RecordFrame(const QString&);
    ~RecordFrame() override {};
    static VK_Manager* manager;
};


class RecordPreview : public RecordBase
{
    Q_OBJECT
public:
    RecordPreview(const Record&, int, const QDateTime&);
    ~RecordPreview() override {}
    void set_gallery_view() override {};
    void set_list_view() override;
//    static VK_Manager* manager;
    static QVector<Record>* records;
    static QMap<int, int>* logs;
//    void mouseDoubleClickEvent(QMouseEvent*) override;
    void reroll();
    void input_number();
private:
//    Record record;
    QList<RecordFrame*> images;
    QLabel log_info;
    QDateTime time;
    QPushButton* reroll_button = new QPushButton(QIcon(":/images/icons8-available-updates-80.png"), "");
    QPushButton* number_button = new QPushButton(QIcon(":/images/icons8-12-80.png"), "");
    void clear();
    void set_index(int);
    void update_log_info(int);
};

#endif // RECORD_ITEMS_H
