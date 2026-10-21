#pragma once
#include <QStringList>
namespace terlox {
struct ReportContext { QString renderer, profile, executable, directory, backdrop, surface; QStringList profiles, arguments; };
QString informationReport(const ReportContext& context = {});
}
