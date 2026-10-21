#include "SetupPages.h"
#include "SetupWizard.h"
#include "installer/backend/InstallEngine.h"
#include <QLineEdit>
#include <QCheckBox>
#include <QLabel>
#include <QPushButton>
#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QFileDialog>
#include <QMessageBox>
#include <QProgressBar>
#include <QThread>
#include <QTimer>
#include <QAbstractButton>
namespace terlox::setup {
DestinationPage::DestinationPage(QWidget* parent):QWizardPage(parent) {
    setTitle("Choose an installation folder"); setSubTitle("Use a dedicated folder for Terlox.");
    auto* layout=new QVBoxLayout(this); layout->addWidget(new QLabel("Installation folder"));
    auto* row=new QHBoxLayout; directory_=new QLineEdit(defaultDestination()); directory_->setObjectName("installationFolder");
    registerField("destination",directory_); row->addWidget(directory_); auto* browse=new QPushButton("Browse…"); row->addWidget(browse); layout->addLayout(row);
    connect(directory_,&QLineEdit::textChanged,this,&QWizardPage::completeChanged);
    connect(browse,&QPushButton::clicked,this,[this] { const auto path=QFileDialog::getExistingDirectory(this,"Install Terlox",directory_->text()); if (!path.isEmpty()) directory_->setText(path); });
    auto* note=new QLabel("An empty folder is recommended. For an upgrade, close all running copies of the installed application first. Setup does not overwrite unrelated files."); note->setWordWrap(true); layout->addWidget(note); layout->addStretch();
}
bool DestinationPage::isComplete() const { return !directory_->text().trimmed().isEmpty(); }
bool DestinationPage::validatePage() {
    InstallPlan plan; QString error; auto options=static_cast<SetupWizard*>(wizard())->options(); options.desktopIntegration=false; options.defaultTerminal=false;
    if (buildInstallPlan(options,plan,error)) return true;
    QMessageBox::warning(this,"Choose another installation folder",error); return false;
}
IntegrationPage::IntegrationPage(QWidget* parent):QWizardPage(parent) {
    setTitle("Shortcuts and integration"); setSubTitle("These choices apply only to your account."); auto* layout=new QVBoxLayout(this);
#ifdef Q_OS_WIN
    auto* integration=new QCheckBox("Add Start menu, Run and Explorer Open in Terlox integration");
#elif defined(Q_OS_MACOS)
    auto* integration=new QCheckBox("Register Terlox with Finder and Launch Services");
#else
    auto* integration=new QCheckBox("Add Terlox to the application launcher");
#endif
    integration->setChecked(true); registerField("integration",integration); layout->addWidget(integration);
    auto* desktop=new QCheckBox("Create a desktop shortcut"); registerField("desktop",desktop); layout->addWidget(desktop);
#ifdef Q_OS_MACOS
    desktop->setEnabled(false);
#endif
    auto* preferred=new QCheckBox("Use Terlox as the default terminal"); preferred->setObjectName("defaultTerminal"); preferred->setEnabled(canSelectDefaultTerminal()); registerField("default",preferred); layout->addWidget(preferred);
    auto* explanation=new QLabel(defaultTerminalExplanation()); explanation->setWordWrap(true); layout->addWidget(explanation);
    connect(integration,&QCheckBox::toggled,this,[=](bool enabled) {
#ifndef Q_OS_MACOS
        desktop->setEnabled(enabled);
#endif
        preferred->setEnabled(enabled && canSelectDefaultTerminal()); if (!enabled) { desktop->setChecked(false); preferred->setChecked(false); }
    }); layout->addStretch();
}
ReviewPage::ReviewPage(QWidget* parent):QWizardPage(parent) {
    setTitle("Ready to install"); setSubTitle("Review your choices before installing."); setCommitPage(true);
    auto* layout=new QVBoxLayout(this); summary_=new QLabel; summary_->setWordWrap(true); summary_->setTextFormat(Qt::PlainText); layout->addWidget(summary_); layout->addStretch();
}
void ReviewPage::initializePage() {
    InstallPlan plan; QString error; valid_=buildInstallPlan(static_cast<SetupWizard*>(wizard())->options(),plan,error);
    if (!valid_) summary_->setText(error);
    else summary_->setText(QString("Terlox %1\n\nInstall to\n%2\n\n%3 files · %4 MB\n\nDesktop integration: %5\nDesktop shortcut: %6\nDefault terminal: %7\n\nClick Install Terlox to begin.").arg(plan.version,plan.options.destination).arg(plan.files.size()).arg(plan.bytes/1048576.,0,'f',1).arg(plan.options.desktopIntegration?"Yes":"No",plan.options.desktopShortcut?"Yes":"No",plan.options.defaultTerminal?"Select Terlox for this account":"Unchanged"));
    emit completeChanged();
}
bool ReviewPage::isComplete() const { return valid_; }
InstallPage::InstallPage(QWidget* parent):QWizardPage(parent) {
    setTitle("Installing Terlox"); setSubTitle("Copying the application and its runtime files."); auto* layout=new QVBoxLayout(this);
    status_=new QLabel; status_->setWordWrap(true); status_->setTextFormat(Qt::PlainText); progress_=new QProgressBar; progress_->setRange(0,100); layout->addWidget(status_); layout->addWidget(progress_); layout->addStretch();
}
void InstallPage::initializePage() {
    if (started_) return; started_=true; auto* setup=static_cast<SetupWizard*>(wizard()); setup->setInstalling(true);
    setup->button(QWizard::BackButton)->setEnabled(false); setup->button(QWizard::CancelButton)->setEnabled(false);
    const auto options=setup->options();
    auto* worker=QThread::create([this,setup,options] {
        InstallPlan plan; QString error; InstallResult result;
        if (!buildInstallPlan(options,plan,error)) result.message=error;
        else result=installPackage(plan,[this](int value,const QString& path) { QMetaObject::invokeMethod(this,[this,value,path] { progress_->setValue(value); status_->setText(path); },Qt::QueuedConnection); });
        QMetaObject::invokeMethod(this,[this,setup,result] {
            setup->setInstalling(false); setup->setResult(result.success,result.message,result.notes);
            finished_=true; emit completeChanged(); setup->next();
        },Qt::QueuedConnection);
    });
    connect(worker,&QThread::finished,worker,&QObject::deleteLater); worker->start();
}
}
