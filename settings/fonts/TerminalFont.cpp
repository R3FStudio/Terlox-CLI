#include "TerminalFont.h"
#include <QFontDatabase>
#include <QGuiApplication>
static void initializeTerminalFontResources() { Q_INIT_RESOURCE(TerminalFonts); }
namespace terlox {
QFont defaultTerminalFont() {
    static bool loaded=false;
    if (!loaded && qobject_cast<QGuiApplication*>(QCoreApplication::instance())) {
        initializeTerminalFontResources();
        for (const auto* face:{"Regular","Bold","Italic","BoldItalic"})
            QFontDatabase::addApplicationFont(QString(":/terlox/fonts/CascadiaMono-%1.ttf").arg(face));
        loaded=true;
    }
    QFont font; font.setFamilies({"Cascadia Mono","Consolas","DejaVu Sans Mono","monospace"});
    font.setPointSizeF(12); font.setStyleHint(QFont::Monospace); font.setFixedPitch(true);
    font.setHintingPreference(QFont::PreferFullHinting); return font;
}
}
