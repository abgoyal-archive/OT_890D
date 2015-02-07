

#ifndef QWEBPLUGINDATABASE_P_H
#define QWEBPLUGINDATABASE_P_H

#include "qwebkitglobal.h"

#include <wtf/RefPtr.h>

namespace WebCore {
    class PluginPackage;
    class PluginDatabase;
};

class QWebPluginInfoPrivate {
public:
    QWebPluginInfoPrivate(RefPtr<WebCore::PluginPackage> pluginPackage);

    RefPtr<WebCore::PluginPackage> plugin;
};

class QWebPluginDatabasePrivate {
public:
    QWebPluginDatabasePrivate(WebCore::PluginDatabase* pluginDatabase);

    WebCore::PluginDatabase* database;
};

#endif // QWEBPLUGINDATABASE_P_H
