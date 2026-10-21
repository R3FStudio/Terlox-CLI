#pragma once
#include "installer/model/InstallPlan.h"
#include <QWizard>
namespace terlox::setup {
class SetupWizard : public QWizard {
public:
    enum Page { Welcome,Destination,Integration,Review,Install,Finish };
    explicit SetupWizard(QString source,QWidget* parent=nullptr,bool registerApplication=true);
    InstallOptions options() const;
    QString source() const { return source_; }
    bool installing() const { return installing_; }
    void setInstalling(bool value) { installing_=value; }
    void setResult(bool success,QString message,QStringList notes);
    bool succeeded() const { return succeeded_; }
    QString resultText() const { return resultText_; }
    void reject() override;
    void accept() override;
private:
    QString source_,resultText_;
    bool installing_=false,succeeded_=false;
    bool registerApplication_=true;
};
}
