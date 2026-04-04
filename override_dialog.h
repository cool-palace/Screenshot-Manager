#ifndef OVERRIDE_DIALOG_H
#define OVERRIDE_DIALOG_H

#include <QDialog>
#include "ui_override_dialog.h"

class OverrideDialog : public QDialog, public Ui_OverrideDialog
{
    Q_OBJECT

public:
    explicit OverrideDialog(const QString& title, const QString& filename, QWidget *parent = nullptr);
    ~OverrideDialog() {}

    QString pattern() const { return lePattern->text(); }
    QString title() const { return leTitle->text(); }

private:
    void validateAndAccept();
};

#endif // OVERRIDE_DIALOG_H
