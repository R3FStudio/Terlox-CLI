#include "ProfileDialog.h"
#include <QDialog>
#include <QDialogButtonBox>
#include <QFormLayout>
#include <QLineEdit>
#include <QPlainTextEdit>
#include <QDir>
#include <QMessageBox>
namespace terlox {
std::optional<Profile> editProfile(QWidget* parent) {
    QDialog dialog(parent); dialog.setWindowTitle("New shell profile"); QFormLayout layout(&dialog);
    QLineEdit name, executable, directory(QDir::homePath()); QPlainTextEdit arguments;
    arguments.setPlaceholderText("One argument per line; no shell quoting"); arguments.setMaximumHeight(100);
    layout.addRow("Name", &name); layout.addRow("Executable", &executable); layout.addRow("Arguments", &arguments); layout.addRow("Working directory", &directory);
    QDialogButtonBox buttons(QDialogButtonBox::Save | QDialogButtonBox::Cancel); layout.addRow(&buttons);
    QObject::connect(&buttons, &QDialogButtonBox::accepted, &dialog, [&] { if (name.text().trimmed().isEmpty() || executable.text().trimmed().isEmpty()) { QMessageBox::warning(&dialog, "Profile", "Enter a name and executable."); return; } dialog.accept(); });
    QObject::connect(&buttons, &QDialogButtonBox::rejected, &dialog, &QDialog::reject);
    if (dialog.exec() != QDialog::Accepted) return {};
    const auto text = arguments.toPlainText();
    return Profile{name.text(), {executable.text(), text.isEmpty() ? QStringList{} : text.split('\n'), directory.text()}};
}
}
