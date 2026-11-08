#include "System.h"
#include "information/Version.h"
#include <QCoreApplication>
#include <QSysInfo>
#include <QThread>
#include <QStorageInfo>
#include <QFile>
#ifdef Q_OS_WIN
#include <windows.h>
#elif defined(Q_OS_MACOS)
#include <sys/sysctl.h>
#else
#include <unistd.h>
#endif
namespace terlox {
QString systemSummary() {
    quint64 memory=0;
#ifdef Q_OS_WIN
    MEMORYSTATUSEX status{}; status.dwLength=sizeof(status);
    if (GlobalMemoryStatusEx(&status)) memory=status.ullTotalPhys;
#elif defined(Q_OS_MACOS)
    size_t size=sizeof(memory); sysctlbyname("hw.memsize",&memory,&size,nullptr,0);
#else
    const auto pages=sysconf(_SC_PHYS_PAGES), pageSize=sysconf(_SC_PAGESIZE);
    if (pages>0 && pageSize>0) memory=quint64(pages)*quint64(pageSize);
#endif
    QString text=QString("Terlox %1\n").arg(TERLOX_VERSION);
    text+="  /\\   TERLOX\n /__\\  Built-in system report\n\n";
    text+="OS: "+QSysInfo::prettyProductName()+"\n";
    text+="Host: "+QSysInfo::machineHostName()+"\n";
    text+="Kernel: "+QSysInfo::kernelType()+" "+QSysInfo::kernelVersion()+"\n";
    text+=QString("CPU: %1 (%2 logical threads)\n").arg(QSysInfo::currentCpuArchitecture()).arg(QThread::idealThreadCount());
    if (memory) text+=QString("Memory: %1 GiB installed\n").arg(double(memory)/(1024.*1024*1024),0,'f',1);
    const auto disk=QStorageInfo::root();
    if (disk.isValid() && disk.bytesTotal()>0)
        text+=QString("Root disk: %1 / %2 GiB available\n").arg(double(disk.bytesAvailable())/(1024.*1024*1024),0,'f',1).arg(double(disk.bytesTotal())/(1024.*1024*1024),0,'f',1);
    text+=QString("Qt: %1 (built with %2)\nBuild: %3\n").arg(qVersion(),QT_VERSION_STR,QSysInfo::buildCpuArchitecture());
    text+="Scrollback: 32 MiB RAM cache per session; older text reloads from temporary disk storage\n";
    text+="Output queue: 1 MiB per session with producer backpressure\n";
    text+="Renderer: live device and session details are in F1 > System\n";
    return text;
}
}
