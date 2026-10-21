#include "GraphicsBackend.h"
#include <QQuickWindow>
#include <QSGRendererInterface>
#ifdef Q_OS_WIN
#include <d3d11.h>
#include <dxgi.h>
#endif
namespace terlox {
void configureGraphics() {
    static const bool configured=[] {
        if (!qEnvironmentVariableIsEmpty("QT_QUICK_BACKEND")) return true;
#ifdef Q_OS_WIN
        const auto requested=qEnvironmentVariable("QSG_RHI_BACKEND").toLower();
        if (requested=="vulkan" || requested=="d3d12") {
            // Qt 6.8's Windows Vulkan presentation failed the native alpha test.
            // Keep the tested transparent composition path for this window type.
            qunsetenv("QSG_RHI_BACKEND");
            qWarning("Terlox: using Direct3D 11 on Windows to preserve real desktop transparency.");
        } else if (!requested.isEmpty()) return true;
        QQuickWindow::setGraphicsApi(QSGRendererInterface::Direct3D11);
#elif defined(Q_OS_MACOS)
        if (!qEnvironmentVariableIsEmpty("QSG_RHI_BACKEND")) return true;
        QQuickWindow::setGraphicsApi(QSGRendererInterface::Metal);
#else
        if (!qEnvironmentVariableIsEmpty("QSG_RHI_BACKEND")) return true;
        QQuickWindow::setGraphicsApi(QSGRendererInterface::OpenGL);
#endif
        return true;
    }(); Q_UNUSED(configured);
}
QString graphicsDescription(QQuickWindow* window) {
    if (!window) return "not initialized";
    const auto* renderer=window->rendererInterface();
    QString api;
    switch (renderer->graphicsApi()) {
    case QSGRendererInterface::Direct3D11: api="Direct3D 11"; break;
    case QSGRendererInterface::Direct3D12: api="Direct3D 12"; break;
    case QSGRendererInterface::Metal: api="Metal"; break;
    case QSGRendererInterface::Vulkan: api="Vulkan"; break;
    case QSGRendererInterface::OpenGL: api="OpenGL"; break;
    case QSGRendererInterface::Software: return "Software scene graph (GPU acceleration disabled)";
    default: return "scene graph not initialized";
    }
#ifdef Q_OS_WIN
    if (renderer->graphicsApi()==QSGRendererInterface::Direct3D11) {
        auto* device=static_cast<ID3D11Device*>(renderer->getResource(window,QSGRendererInterface::DeviceResource));
        if (device) { IDXGIDevice* dxgi=nullptr;
            if (SUCCEEDED(device->QueryInterface(__uuidof(IDXGIDevice),reinterpret_cast<void**>(&dxgi)))) {
                IDXGIAdapter* adapter=nullptr;
                if (SUCCEEDED(dxgi->GetAdapter(&adapter))) { DXGI_ADAPTER_DESC desc{}; if (SUCCEEDED(adapter->GetDesc(&desc))) api += " - " + QString::fromWCharArray(desc.Description); adapter->Release(); }
                dxgi->Release();
            }
        }
    }
#endif
    return api + " (Qt RHI scene graph)";
}
}
