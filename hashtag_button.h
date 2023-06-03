#ifndef HASHTAGBUTTON_H
#define HASHTAGBUTTON_H
#include <QPushButton>
#include <QMouseEvent>

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
