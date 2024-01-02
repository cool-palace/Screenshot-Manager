#ifndef HASHTAGBUTTON_H
#define HASHTAGBUTTON_H
#include <QPushButton>
#include <QMouseEvent>
#include <QDateTime>
#include <QJsonObject>
#include <QGridLayout>
#include <QLabel>
#include <QLineEdit>

class Hashtag {
public:
    Hashtag(const QString&, const QJsonObject&);
    Hashtag() {};
    QString text() const;
    QString option() const;
    QString description() const { return descr; }
    QString tag() const { return name; }
    QDateTime last_poll() const { return poll; }
private:
    QString name;
    int rank;
    QString emoji;
    QString descr;
    QDateTime poll;
    QDateTime won;
};

class HashtagPreview : public QWidget
{
    Q_OBJECT
public:
    HashtagPreview(const Hashtag&);
    ~HashtagPreview() override { --total; }
    void set_hashtag(const Hashtag&);
    QString line() const { return hashtag.text(); }
    QString option() const { return hashtag.option(); }
    QString text_description() const { return (description.text().isEmpty() ? hashtag.description() : description.text()) + '\n' ; }
    void update_log_info();
    static QMap<QString, HashtagPreview*>* selected_hashtags;
//    void mouseDoubleClickEvent(QMouseEvent*) override;
signals:
    void reroll_request(const QString&);
private:
    int index;
    Hashtag hashtag;
    QLabel text;
    QLabel number;
    QGridLayout layout;
    static int total;
    QLabel log_info;
    QLineEdit description;
    QPushButton* reroll_button = new QPushButton(QIcon(":/images/icons8-available-updates-80.png"), "");
    void reroll();
};

class HashtagButton : public QPushButton
{
    Q_OBJECT
public:
    HashtagButton(const QString&);
    ~HashtagButton() override {}
    void mousePressEvent(QMouseEvent*) override;
    void highlight(const QChar&, bool);
    void highlight(bool, bool);
    void highlight_unregistered();
    void show_count();
    void reset();
    void add_index(const QChar&, int);
    void remove_index(const QChar&, int);
    QSet<int> indices(const QChar&, bool) const;
    static void update_on_records(int);
signals:
    void filterEvent(const QChar&, const QString&, bool);
    void hashtagEvent(const QChar&, const QString&);
private:
    QString text;
    int count = 0;
    QMap<QChar, QSet<int>> record_indices;
    static int records_size;
};

#endif // HASHTAGBUTTON_H
