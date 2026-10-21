#include "SetupWizard.h"
#include "SetupPages.h"
#include "branding/Brand.h"
#include <QCheckBox>
#include <QLabel>
#include <QVBoxLayout>
#include <QProcess>
#include <QDir>
#include <QMessageBox>
#include <QAbstractButton>
#include <QPalette>
#include <QFont>
namespace terlox::setup {
namespace {
class FinishPage : public QWizardPage {
public:
    FinishPage() {
        setTitle("Terlox is ready"); auto* layout=new QVBoxLayout(this);
        result_=new QLabel; result_->setWordWrap(true); result_->setTextFormat(Qt::PlainText); layout->addWidget(result_);
        launch_=new QCheckBox("Launch Terlox when setup closes"); launch_->setChecked(true); registerField("launch",launch_); layout->addWidget(launch_); layout->addStretch();
    }
    void initializePage() override {
        auto* setup=static_cast<SetupWizard*>(wizard()); result_->setText(setup->resultText());
        launch_->setEnabled(setup->succeeded()); launch_->setChecked(setup->succeeded());
        setTitle(setup->succeeded()?"Terlox is ready":"Installation needs attention");
        wizard()->button(QWizard::BackButton)->setEnabled(false);
    }
private:
    QLabel* result_; QCheckBox* launch_;
};
}
SetupWizard::SetupWizard(QString source,QWidget* parent,bool registerApplication):QWizard(parent),source_(std::move(source)),registerApplication_(registerApplication) {
    setWindowTitle("Terlox Setup"); setWindowIcon(brandIcon());
    setFont(QFont("Arial",10));
    auto colors=palette();
    colors.setColor(QPalette::Window,QColor("#141219")); colors.setColor(QPalette::Base,QColor("#141219"));
    colors.setColor(QPalette::WindowText,QColor("#f6edf1")); colors.setColor(QPalette::Text,QColor("#f6edf1"));
    colors.setColor(QPalette::Button,QColor("#33232e")); colors.setColor(QPalette::ButtonText,QColor("#f6edf1"));
    colors.setColor(QPalette::Highlight,QColor("#b95d78")); colors.setColor(QPalette::HighlightedText,Qt::white); setPalette(colors);
    setWizardStyle(QWizard::ModernStyle); setMinimumSize(680,490);
    setOption(QWizard::NoBackButtonOnStartPage); setOption(QWizard::NoBackButtonOnLastPage);
    setOption(QWizard::NoCancelButtonOnLastPage); setButtonText(QWizard::CommitButton,"Install Terlox");
    setPixmap(QWizard::LogoPixmap,brandIcon().pixmap(48,48));
    setStyleSheet("QWizard { background:#141219; color:#f6edf1; } QWizardPage { background:#141219; } QLabel,QCheckBox { color:#f6edf1; background:transparent; } QLineEdit { background:#211b25; color:#fff4f8; border:1px solid #65404e; border-radius:6px; padding:8px; } QPushButton { background:#33232e; color:#fff4f8; border:1px solid #805363; border-radius:6px; padding:7px 16px; } QPushButton:hover { background:#50323f; } QPushButton:disabled { color:#82767d; border-color:#3d3239; } QProgressBar { border:1px solid #67424f; border-radius:5px; text-align:center; color:#fff; } QProgressBar::chunk { background:#b95d78; }");
    auto* welcome=new QWizardPage; welcome->setTitle("Install Terlox"); welcome->setSubTitle("Choose a folder and the shortcuts you want.");
    auto* layout=new QVBoxLayout(welcome); auto* copy=new QLabel("Setup installs Terlox for your account.\n\nYou'll choose an installation folder, select desktop shortcuts, and review everything before files are copied.\n\nYour existing terminal preferences are kept.");
    copy->setWordWrap(true); layout->addWidget(copy); layout->addStretch();
    setPage(Welcome,welcome); setPage(Destination,new DestinationPage); setPage(Integration,new IntegrationPage);
    setPage(Review,new ReviewPage); setPage(Install,new InstallPage); setPage(Finish,new FinishPage);
}
InstallOptions SetupWizard::options() const {
    return {source_,field("destination").toString(),field("integration").toBool(),field("desktop").toBool(),field("default").toBool(),registerApplication_};
}
void SetupWizard::setResult(bool success,QString message,QStringList notes) {
    succeeded_=success; resultText_=std::move(message);
    if (!notes.isEmpty()) resultText_+="\n\n"+notes.join("\n\n");
}
void SetupWizard::reject() {
    if (!installing_) QWizard::reject();
}
void SetupWizard::accept() {
    if (succeeded_ && field("launch").toBool()) {
        InstallPlan plan; QString error;
        // The installed marker supplies the launch path; source is still the extracted package.
        if (!buildInstallPlan(options(),plan,error) || !QProcess::startDetached(QDir(plan.options.destination).filePath(plan.executable),{},plan.options.destination)) {
            QMessageBox::warning(this,"Terlox Setup","Terlox was installed, but could not be launched. Open it from its installation folder.");
        }
    }
    QWizard::accept();
}
}
