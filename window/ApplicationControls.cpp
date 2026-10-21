#include "TerminalView.h"
#include <cmath>
namespace terlox {
void TerminalView::applyApplicationControls(const QJsonObject& controls) {
    const auto previous=appControls_;
    if (controls.value("reset").toBool()) appControls_={};
    for (const auto& name:{QString("opacity"),QString("scale"),QString("blur")}) if (controls.value(name).isDouble()) {
        const auto value=controls[name].toDouble(); const double low=name=="scale"?.5:0., high=name=="scale"?3.:1.;
        if (std::isfinite(value) && value>=low && value<=high) appControls_[name]=value;
    }
    if (controls.value("fontFamily").isString()) {
        const auto family=controls["fontFamily"].toString();
        if (!family.trimmed().isEmpty() && family.size()<=128) appControls_["fontFamily"]=family;
    }
    if (controls.value("borderColor").isString() && QColor(controls["borderColor"].toString()).isValid())
        appControls_["borderColor"]=controls["borderColor"];
    // Pen-only changes are already stored in Screen. They must not resize the
    // PTY or rebuild window effects for every translucent letter an app prints.
    if (appControls_==previous) return;
    applyEffectiveAppearance(); emit surfaceAppearanceChanged();
}
void TerminalView::setAppearance(const Appearance& appearance) { baseAppearance_=appearance; applyEffectiveAppearance(); }
}
