#include "Commands.h"
#include <cmath>
namespace terlox {
QString customizationHelp() {
    return "About Terlox                    Interactive terminal About/help app\n"
           "/terlox                         Show current settings\n"
           "/terlox update                  Check the latest public Terlox release\n"
           "/terlox blur 0.85               Native background blur (0..1; 0 disables)\n"
           "/terlox effects on|off          Native acrylic/vibrancy effects\n"
           "/terlox opacity 0.45            Black tint opacity (0.2..1)\n"
           "/terlox border #ba5666          Window outline color\n"
           "/terlox font \"Cascadia Mono\" 12 Font family and optional size (6..40)\n"
           "/terlox scrollback 10000        History lines for new sessions\n"
           "/terlox default-shell \"name\"   Set the default shell profile\n"
           "/terlox profile-add             Add a shell profile\n"
           "/terlox reset                   Restore default appearance\n"
           "/admin [command]                Request Windows UAC / open elevated Terlox\n"
           "Windows terloxsh: admin [command] preserves the shell's current directory.\n"
           "Drag the blank top strip to move; terminal content only selects text.\n"
           "Native blur radius is OS-controlled; blur sets the material intensity.";
}
AppearanceResult appearanceCommand(const QStringList& args, Appearance& appearance) {
    if (args.isEmpty()) return {false,QString("blur=%1 effects=%2 opacity=%3 border=%4 font=%5 %6 scrollback=%7\n/terlox help lists all commands.").arg(appearance.blur).arg(appearance.effects?"on":"off").arg(appearance.opacity).arg(appearance.border.name()).arg(appearance.font.family()).arg(appearance.font.pointSizeF()).arg(appearance.scrollback)};
    const auto name=args[0].toLower();
    if (name=="help" && args.size()==1) return {false,customizationHelp()};
    if (name=="reset" && args.size()==1) { appearance=Appearance{}; return {true,"Default appearance restored (blur 0.85)."}; }
    auto invalid=[] { return AppearanceResult{false,"Invalid setting/value. Use /terlox help for valid commands and ranges."}; };
    if (name=="font" && (args.size()==2 || args.size()==3) && !args[1].trimmed().isEmpty()) {
        bool valid=true; const auto size=args.size()==3?args[2].toDouble(&valid):appearance.font.pointSizeF();
        if (!valid || !std::isfinite(size) || size<6 || size>40) return invalid();
        appearance.font.setFamily(args[1]); appearance.font.setPointSizeF(size);
    } else if (args.size()!=2) return invalid();
    else if (name=="effects") {
        if (args[1]!="on" && args[1]!="off") return invalid(); appearance.effects=args[1]=="on";
    } else if (name=="border") {
        const QColor color(args[1]); if (!color.isValid()) return invalid(); appearance.border=color;
    } else {
        bool valid=false; const auto value=args[1].toDouble(&valid);
        if (!valid || !std::isfinite(value)) return invalid();
        if (name=="opacity" && value>=.2 && value<=1) appearance.opacity=value;
        else if (name=="blur" && value>=0 && value<=1) appearance.blur=value;
        else if (name=="scrollback" && value>=0 && value<=100000 && value==std::floor(value)) appearance.scrollback=int(value);
        else return invalid();
    }
    return {true,"Updated "+name+". Settings saved."};
}
}
