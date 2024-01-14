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
    QJsonObject to_json() const;
    void set_description(const QString& new_descr) { descr = new_descr; }
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
    QString text_description() const { return description.text().isEmpty() ? hashtag.description() : description.text(); }
    void update_log_info();
    static QMap<QString, HashtagPreview*>* selected_hashtags;
    static QMap<QString, int>* poll_logs;
    bool is_edited() const { return edited; }
//    void mouseDoubleClickEvent(QMouseEvent*) override;
signals:
    void reroll_request(const QString&);
    void search_start(const QString&);
private:
    int index;
    Hashtag hashtag;
    QLabel text;
    QLabel number;
    QGridLayout layout;
    static int total;
    QLabel log_info;
    QLineEdit description;
    bool edited = false;
    QPushButton* reroll_button = new QPushButton(QIcon(":/images/icons8-available-updates-80.png"), "");
    QPushButton* search_button = new QPushButton(QIcon(":/images/icons8-search-80.png"), "");
    void reroll();
    void search();
};

class HashtagButton : public QPushButton
{
    Q_OBJECT
public:
    HashtagButton(const QString&);
    ~HashtagButton() override {}
    void mousePressEvent(QMouseEvent*) override;
    void mouseDoubleClickEvent(QMouseEvent*) override;
    void highlight(const QChar&, bool);
    void highlight(bool, bool);
    void highlight_unregistered();
    void show_count();
    void reset();
    void add_index(const QChar&, int);
    void remove_index(const QChar&, int);
    QSet<int> indices(const QChar&, bool) const;
    static void update_on_records(int);
    static void set_preview_to_change(HashtagPreview*);
    static HashtagPreview* current_preview_to_change();
signals:
    void filterEvent(const QChar&, const QString&, bool);
    void hashtagEvent(const QChar&, const QString&);
    void selected(const QString&, HashtagPreview*);
private:
    QString text;
    int count = 0;
    QMap<QChar, QSet<int>> record_indices;
    static int records_size;
    static HashtagPreview* preview_to_change;
};

#endif // HASHTAGBUTTON_H
