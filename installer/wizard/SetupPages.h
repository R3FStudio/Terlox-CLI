#pragma once
#include <QWizardPage>
class QLineEdit;
class QCheckBox;
class QLabel;
class QProgressBar;
namespace terlox::setup {
class DestinationPage : public QWizardPage {
public:
    explicit DestinationPage(QWidget* parent=nullptr);
    bool isComplete() const override;
    bool validatePage() override;
private:
    QLineEdit* directory_;
};
class IntegrationPage : public QWizardPage {
public:
    explicit IntegrationPage(QWidget* parent=nullptr);
};
class ReviewPage : public QWizardPage {
public:
    explicit ReviewPage(QWidget* parent=nullptr);
    void initializePage() override;
    bool isComplete() const override;
private:
    QLabel* summary_;
    bool valid_=false;
};
class InstallPage : public QWizardPage {
    Q_OBJECT
public:
    explicit InstallPage(QWidget* parent=nullptr);
    void initializePage() override;
    bool isComplete() const override { return finished_; }
private:
    QProgressBar* progress_;
    QLabel* status_;
    bool started_=false,finished_=false;
};
}
