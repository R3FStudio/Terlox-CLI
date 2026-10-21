#include "Brand.h"
static void initializeBrand() { Q_INIT_RESOURCE(Brand); }
namespace terlox {
QIcon brandIcon() { initializeBrand(); QIcon icon; for (const int size:{16,24,32,48,64,128,256,512,1024}) icon.addFile(QString(":/terlox/icons/terlox-%1.png").arg(size)); return icon; }
}
