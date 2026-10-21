#pragma once
#include "Appearance.h"
#include "Profiles.h"
#include <QSettings>
namespace terlox {
class Store {
public:
    Appearance appearance() const;
    void saveAppearance(const Appearance& appearance);
    QList<Profile> profiles(const QString& appDir) const;
    void addProfile(const Profile& profile);
    QString defaultProfile() const;
    void setDefaultProfile(const QString& name);
private:
    mutable QSettings settings_;
};
}
