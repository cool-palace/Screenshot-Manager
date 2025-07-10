#ifndef COMMON_H
#define COMMON_H
#include <QJsonObject>
#include <QJsonDocument>
#include <QFile>
#include <QDialog>
#include <QDialogButtonBox>
#include <QVBoxLayout>
#include <QLabel>
#include <QLineEdit>
#include <QProgressBar>
#include <QPushButton>

class CaptchaDialog : public QDialog {
    Q_OBJECT
public:
    CaptchaDialog(QWidget* parent = nullptr);
    ~CaptchaDialog();
    QString text() const { return line_edit.text(); }
public slots:
    void set_captcha_image(const QImage&);
private:
    QDialogButtonBox* buttons = new QDialogButtonBox(QDialogButtonBox::Ok | QDialogButtonBox::Cancel);
    QVBoxLayout layout;
    QLabel captcha;
    QLineEdit line_edit;
};

class ProgressDialog : public QDialog {
    Q_OBJECT
public:
    ProgressDialog(QWidget *parent = nullptr);
    ~ProgressDialog();
public slots:
    void update_progress(int value, const QString& text);

private:
    QDialogButtonBox* buttons = new QDialogButtonBox(QDialogButtonBox::Ok | QDialogButtonBox::Abort);
    QVBoxLayout layout;
    QProgressBar progress;
    QLabel text;
};

void clear_grid(QLayout* layout, bool hide = true);
double roundTo(double value, int decimals);
QJsonObject json_object(const QString& filepath);
bool save_json(const QJsonObject& object, QFile& file);
QStringList word_forms(const QString&);
QString inflect(int, const QString&);
int lowest_degree_vertex(const QList<QList<int>>& M);
void find_hamiltonian_cycles(int current, const QList<QList<int>>& M, QVector<int>& path, QSet<int>& visited, QList<QVector<int>>& cycles, int start);
QList<QVector<int>> get_all_hamiltonian_cycles(const QList<QList<int>>& M);
QList<QVector<int>> remove_duplicate_cycles(const QList<QVector<int>>& cycles);

#endif // COMMON_H
