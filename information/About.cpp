#include "About.h"
#include "Shortcuts.h"
#include "information/Version.h"
#include <algorithm>
#include <QSysInfo>
namespace terlox {
QByteArray aboutPage(int columns,int rows,int page,int scroll,const QString& report,int* actualScroll) {
    const int width=std::max(12,std::min(columns-4,100));
    QString out="\033[?1049h\033[?25l\033[48;2;10;14;23m\033[2J\033[H";
    auto line=[&](QString text,const QString& color="\033[38;2;188;201;220m") { text=text.left(width); out+="  "+color+text+"\033[0m\033[48;2;10;14;23m\r\n"; };
    line("T E R L O X   /   NATIVE TERMINAL", "\033[1;38;2;246;104;139m");
    line(QString(width,QChar(0x2500)),"\033[38;2;55;79;104m");
    const QStringList names{"01 OVERVIEW","02 KEYS","03 SYSTEM","04 LICENSE"};
    QString tabs; for (int i=0;i<names.size();++i) tabs+=(i==page?"["+names[i]+"]":" "+names[i]+" ")+"  ";
    line(tabs,"\033[38;2;100;221;211m"); line("");
    QStringList body;
    if (page==0) body={
        "  _____  _____  ____   _       ___   __  __",
        " |_   _|| ____||  _ \\ | |     / _ \\  \\/ /",
        "   | |  |  _|  | |_) || |    | | | |  >  <",
        "   | |  | |___ |  _ < | |___ | |_| | / /\\ \\",
        "   |_|  |_____||_| \\_\\|_____| \\___/ /_/  \\_\\",
        "",QString("  VERSION %1  /  C++20 + Qt %2").arg(TERLOX_VERSION,qVersion()),
        "  Author: R3F Studio",
        "", "  A clear space for powerful command-line tools.",
        "  Native processes. GPU text. Real backdrop glass.","",
        "  RGB THEMES        24-bit foreground + background",
        "  APP CONTROLS      per-session opacity + text scale",
        "  GRAPHICS          PNG / RGB / RGBA image placements",
        "  INPUT             mouse, focus, bracketed paste",
        "", "  /terlox help      Appearance and profile commands",
        "  /info-terlox      Full terminal diagnostics",
        "  Right-click       Open an administrator session"};
    else if (page==1) { for (const auto& key:shortcuts()) body << key.keys.leftJustified(22)+key.description; body << "" << "Ctrl + A / C / V   Select all / copy / paste" << "Ctrl + Alt + A/C/V Send raw control keys to app" << "Scroll wheel / bar Smooth terminal history" << "Ctrl + wheel       Zoom terminal text" << "Shift + plus/minus Zoom terminal text" << "Drag text          Select terminal text" << "Shift + drag       Select in mouse-aware apps" << "Shift + right-click Terminal menu in mouse-aware apps" << "Drag top strip     Move window; content selects"; }
    else if (page==2) body=report.split('\n');
    else body={"APACHE LICENSE 2.0", "", "Copyright 2026 R3F Studio", "", "Original Terlox code and documentation: Apache-2.0.", "https://www.apache.org/licenses/LICENSE-2.0", "", "THIRD-PARTY COMPONENTS", "", "libvterm: MIT, copyright Paul Evans and contributors.", "Qt: LGPL/GPL/commercial terms, as distributed.", "", "See LICENSE, NOTICE and licenses/ in the package.", "", "Windows is locally tested. Linux/macOS CI is configured.", "Specific graphics protocols have documented limits.", ""};
    QStringList wrapped;
    for (auto text:body) { while (text.size()>width) { auto cut=text.lastIndexOf(' ',width); if (cut<width/2) cut=width; wrapped << text.left(cut); text=text.mid(cut).trimmed(); } wrapped << text; }
    body=wrapped;
    const int visible=std::max(1,rows-8),start=std::clamp(scroll,0,std::max(0,int(body.size())-visible));
    if (actualScroll) *actualScroll=start;
    for (int i=0;i<visible && start+i<body.size();++i) line(body[start+i],page==0 && start+i<5?"\033[1;38;2;100;221;211m":"\033[38;2;188;201;220m");
    out+="\033["+QString::number(std::max(1,rows-2))+";1H";
    line(QString(width,QChar(0x2500)),"\033[38;2;55;79;104m");
    line("1-4 / Tab: pages   Up/Down: scroll   Q / Esc: return","\033[38;2;246;104;139m");
    return out.toUtf8();
}
}
