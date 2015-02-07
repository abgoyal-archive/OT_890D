

#include <WebKit/npfunctions.h>

extern NPNetscapeFuncs *browser;

typedef struct {
    NPObject header;

    NPP npp;
    NPBool eventLogging;
    NPBool logSetWindow;
    NPBool logDestroy;
    NPBool returnErrorFromNewStream;
    NPObject* testObject;
    NPStream* stream;
    char* onStreamLoad;
    char* onStreamDestroy;
    char* onURLNotify;
    char* firstUrl;
    char* firstHeaders;
    char* lastUrl;
    char* lastHeaders;
#ifdef XP_MACOSX
    NPEventModel eventModel;
#endif
} PluginObject;

extern NPClass *getPluginClass(void);
extern void handleCallback(PluginObject* object, const char *url, NPReason reason, void *notifyData);
extern void notifyStream(PluginObject* object, const char *url, const char *headers);
extern void testNPRuntime(NPP npp);
extern void pluginLog(NPP instance, const char* format, ...);
