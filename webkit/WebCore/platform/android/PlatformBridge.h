

#ifndef PlatformBridge_h
#define PlatformBridge_h

namespace WebCore {

// An interface to the embedding layer, which has the ability to answer
// questions about the system and so on...
// This is very similar to ChromiumBridge and the two are likely to converge
// in the future.
//
// The methods in this class all need to reach across a JNI layer to the Java VM
// where the embedder runs. The JNI machinery is currently all in WebKit/android
// but the long term plan is to move to the WebKit API and share the bridge and its
// implementation with Chromium. The JNI machinery will then move outside of WebKit,
// similarly to how Chromium's IPC layer lives outside of WebKit.
class PlatformBridge {
public:
    // Whether the WebView is paused.
    static bool isWebViewPaused();
};
}
#endif // PlatformBridge_h
