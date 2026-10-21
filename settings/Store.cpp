#include "Store.h"
#include <QJsonArray>
#include <QJsonDocument>
#include <QJsonObject>
#include <algorithm>
namespace terlox {
Appearance Store::appearance() const {
    Appearance a; a.font.fromString(settings_.value("appearance/font", a.font.toString()).toString());
    a.blur=std::clamp(settings_.value("appearance/blur",.85).toDouble(),0.,1.);
    a.effects=settings_.value("appearance/effects",true).toBool();
    a.opacity = std::clamp(settings_.value("appearance/opacity", a.opacity).toDouble(), .2, 1.0);
    a.border = QColor(settings_.value("appearance/border", "#ba5666").toString()); if (!a.border.isValid()) a.border = QColor("#ba5666");
    if (settings_.value("appearance/themeVersion",1).toInt()<2 && a.border==QColor("#82603b")) a.border=QColor("#ba5666");
    if (settings_.value("appearance/themeVersion",1).toInt()<3 && a.opacity>=.76) a.opacity=.68;
    if (settings_.value("appearance/themeVersion",1).toInt()<4) {
        if (a.opacity>=.68) a.opacity=.45;
        a.font=Appearance().font;
    }
    if (settings_.value("appearance/themeVersion",1).toInt()<5 && a.font.family().compare("Arial",Qt::CaseInsensitive)==0)
        a.font=Appearance().font;
    a.scrollback = std::clamp(settings_.value("terminal/scrollback", 10000).toInt(), 0, 100000); return a;
}
void Store::saveAppearance(const Appearance& a) { settings_.setValue("appearance/themeVersion",5); settings_.setValue("appearance/blur",a.blur); settings_.setValue("appearance/effects",a.effects); settings_.setValue("appearance/font", a.font.toString()); settings_.setValue("appearance/opacity", a.opacity); settings_.setValue("appearance/border", a.border.name()); settings_.setValue("terminal/scrollback", a.scrollback); }
QList<Profile> Store::profiles(const QString& appDir) const {
    auto profiles = discoverProfiles(appDir);
    const auto array = QJsonDocument::fromJson(settings_.value("profiles/custom").toByteArray()).array();
    for (const auto& v : array) {
        const auto o = v.toObject(); QStringList args; for (const auto& arg : o["arguments"].toArray()) args << arg.toString();
        profiles << Profile{o["name"].toString(), {o["executable"].toString(), args, o["directory"].toString()}};
    }
    return profiles;
}
void Store::addProfile(const Profile& p) {
    auto array = QJsonDocument::fromJson(settings_.value("profiles/custom").toByteArray()).array();
    array.append(QJsonObject{{"name", p.name}, {"executable", p.launch.executable}, {"arguments", QJsonArray::fromStringList(p.launch.arguments)}, {"directory", p.launch.directory}});
    settings_.setValue("profiles/custom", QJsonDocument(array).toJson(QJsonDocument::Compact));
}
QString Store::defaultProfile() const { return settings_.value("profiles/default", "Terlox shell").toString(); }
void Store::setDefaultProfile(const QString& name) { settings_.setValue("profiles/default", name); }
}
