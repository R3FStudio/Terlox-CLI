#pragma once
#include <QByteArray>
#include <QString>
namespace terlox { QByteArray aboutPage(int columns,int rows,int page,int scroll,const QString& report={},int* actualScroll=nullptr); }
