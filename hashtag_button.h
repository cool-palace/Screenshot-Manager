#ifndef HASHTAGBUTTON_H
#define HASHTAGBUTTON_H
#include <QPushButton>
#include <QMouseEvent>
#include <QDateTime>
#include <QJsonObject>
#include <QGridLayout>
#include <QLabel>
#include <QLineEdit>

enum FilterType {
    INCLUDE = 0,
    EXCLUDE = 1,
    HASHTAG = 1 << 1,
    AMPTAG = 1 << 2,
    HASHTAG_EXCLUDE = HASHTAG | EXCLUDE,
    AMPTAG_EXCLUDE = AMPTAG | EXCLUDE,
    ANY_TAG = HASHTAG | AMPTAG,
    ANY_TAG_EXCLUDE = ANY_TAG | EXCLUDE,
    TEXT = 1 << 3,
    TITLE = 1 << 4,
    DATE = 1 << 5,
    PUBLIC = 1 << 6,
    HIDDEN = PUBLIC | EXCLUDE,
    SINGLE = 1 << 7,
    MULTIPLE = SINGLE | EXCLUDE,
    LOGS = 1 << 8,
};

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
    HashtagPreview(const Hashtag& tag = Hashtag());
    ~HashtagPreview() override {}
    void set_hashtag(const Hashtag&);
    QString line() const { return hashtag.text(); }
    QString option() const { return hashtag.option(); }
    QString text_description() const { return description.text().isEmpty() ? hashtag.description() : description.text(); }
    void update_log_info();
    void update_count(int count);
    static QMap<QString, HashtagPreview*>* selected_hashtags;
    static QMap<QString, int>* poll_logs;
    bool is_edited() const { return edited; }
//    void mouseDoubleClickEvent(QMouseEvent*) override;
signals:
    void reroll_request(const QString&);
    void search_start(const QString&);
    void check_request(const QString&);
    void count_request(const QString&);
private:
    int index;
    Hashtag hashtag;
    QLabel text;
    QLabel number;
    QGridLayout layout;
    QLabel log_info;
    QLineEdit description;
    bool edited = false;
    QPushButton* reroll_button = new QPushButton(QIcon(":/images/icons8-available-updates-80.png"), "");
    QPushButton* search_button = new QPushButton(QIcon(":/images/icons8-text-80.png"), "");
    QPushButton* check_button = new QPushButton(QIcon(":/images/icons8-search-80.png"), "");
    void reroll();
    void search();
    void check();
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
    void highlight(FilterType, bool);
    void highlight_unregistered();
    void show_count();
    int current_count() const;
    void show_filtered_count(const QSet<int>&);
    int get_count() const { return all_indices().size(); }
    void reset();
    void add_index(const QChar&, int);
    void remove_index(const QChar&, int);
    QSet<int> indices(FilterType) const;
    QSet<int> all_indices() const { return (record_indices['#'] + record_indices['&']); }
    static void update_on_records(int);
    static void set_preview_to_change(HashtagPreview*);
    static HashtagPreview* current_preview_to_change();
signals:
    void filterEvent(FilterType, const QString&);
    void hashtagEvent(const QChar&, const QString&);
    void selected(const QString&, HashtagPreview*);
private:
    QString text;
    QMap<QChar, QSet<int>> record_indices;
    static int records_size;
    static HashtagPreview* preview_to_change;
};

#endif // HASHTAGBUTTON_H
