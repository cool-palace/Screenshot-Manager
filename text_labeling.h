#ifndef TEXT_LABELING_H
#define TEXT_LABELING_H

//#include "include/abstract_mode.h"
//#include "include/mainwindow.h"
#include "include/common.h"
#include <QJsonObject>
#include <ui_text_labeling.h>

struct LineStyle {
    LineStyle(double y, double h) : y(y), h(h) {}
    double y;
    double h;
    QJsonObject to_json() const {
        QJsonObject current;
        current["y"] = roundTo(y, 3);
        current["h"] = roundTo(h, 3);
        return current;
    }
};

struct XLabel {
    XLabel(double x, int i) : x(x), i(i) {}
    double x;
    int i;
    QJsonObject to_json() const {
        QJsonObject current;
        current["x"] = roundTo(x, 3);
        current["style"] = i;
        return current;
    }
};

struct Label {
    Label(double x, double y, double w, double h) : x(x), y(y), w(w), h(h) {}
    Label(double x, const LineStyle& line) : x(x), y(line.y), w(1 - 2*x), h(line.h) {}
    double x;
    double y;
    double w;
    double h;
};

class TextLabeling : public QWidget, public Ui_TextLabelingWidget {
    Q_OBJECT

public:
    explicit TextLabeling(QWidget *parent = nullptr);

protected:
    QMap<int, QString> locations;
    QStringList pics;
    int pic_index;
    QString title;
    QList<LineStyle> m_styles;
    QList<QList<XLabel>> m_labels; // Индекс соответствует pic_index

protected:
    QPixmap scaled_with_box(QImage source) const;
    QString title_name(int index = 0);

public slots:
    virtual void start();
    virtual void keyPressEvent(QKeyEvent*);
    virtual void keyReleaseEvent(QKeyEvent*);

protected slots:
    virtual void slider_change(int);
    virtual void box_change();
    virtual void style_change(int);
    virtual void back_button();
    virtual void ok_button();
    virtual void skip_button();
    virtual void remove_line_button();
    virtual void edit_style_button();
    virtual void add_line();
    virtual void add_style(bool b = true);
    virtual void add_style(double, double);
    virtual void switch_mode();
    virtual void draw(int);
    virtual void show_status();
    virtual bool data_ready();
    void enable_x(bool);
    void update_style(int);
    void set_enabled(bool);
    void load_labels(const QJsonObject&);
    void save_labels();
    void clear_all();

};

#endif // TEXT_LABELING_H
