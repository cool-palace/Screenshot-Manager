#include "override_dialog.h"
#include <QMessageBox>

OverrideDialog::OverrideDialog(const QString &title, const QString &filename, QWidget *parent) : QDialog(parent) {
    setupUi(this);
    setWindowTitle("Новое правило: " + title);
    lePattern->setText(filename);
    leTitle->setText(filename);

    connect(buttonBox, &QDialogButtonBox::accepted, this, &OverrideDialog::validateAndAccept);
    connect(buttonBox, &QDialogButtonBox::rejected, this, &QDialog::reject);
}

void OverrideDialog::validateAndAccept() {
    if (lePattern->text().isEmpty() || leTitle->text().isEmpty()) {
        QMessageBox::warning(this, "Error", "Все поля должны быть заполнены");
        return;
    }
    accept();
}


