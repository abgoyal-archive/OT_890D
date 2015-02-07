

#ifndef XSSAuditor_h
#define XSSAuditor_h

#include "PlatformString.h"
#include "TextEncoding.h"

namespace WebCore {

    class Frame;
    class ScriptSourceCode;

    // The XSSAuditor class is used to prevent type 1 cross-site scripting
    // vulnerabilites (also known as reflected vulnerabilities).
    //
    // More specifically, the XSSAuditor class decides whether the execution of
    // a script is to be allowed or denied based on the content of any
    // user-submitted data, including:
    //
    // * the query string of the URL.
    // * the HTTP-POST data.
    //
    // If the source code of a script resembles any user-submitted data then it
    // is denied execution.
    //
    // When you instantiate the XSSAuditor you must specify the {@link Frame}
    // of the page that you wish to audit.
    //
    // Bindings
    //
    // An XSSAuditor is instantiated within the contructor of a
    // ScriptController object and passed the Frame the script originated. The
    // ScriptController calls back to the XSSAuditor to determine whether a
    // JavaScript script is safe to execute before executing it. The following
    // methods call into XSSAuditor:
    //
    // * ScriptController::evaluate - used to evaluate JavaScript scripts.
    // * ScriptController::createInlineEventListener - used to create JavaScript event handlers.
    // * HTMLTokenizer::scriptHandler - used to load external JavaScript scripts.
    //
    class XSSAuditor {
    public:
        XSSAuditor(Frame*);
        ~XSSAuditor();

        bool isEnabled() const;

        // Determines whether the script should be allowed or denied execution
        // based on the content of any user-submitted data.
        bool canEvaluate(const String& code) const;

        // Determines whether the JavaScript URL should be allowed or denied execution
        // based on the content of any user-submitted data.
        bool canEvaluateJavaScriptURL(const String& code) const;

        // Determines whether the event listener should be created based on the
        // content of any user-submitted data.
        bool canCreateInlineEventListener(const String& functionName, const String& code) const;

        // Determines whether the external script should be loaded based on the
        // content of any user-submitted data.
        bool canLoadExternalScriptFromSrc(const String& context, const String& url) const;

        // Determines whether object should be loaded based on the content of
        // any user-submitted data.
        //
        // This method is called by FrameLoader::requestObject.
        bool canLoadObject(const String& url) const;

        // Determines whether the base URL should be changed based on the content
        // of any user-submitted data.
        //
        // This method is called by HTMLBaseElement::process.
        bool canSetBaseElementURL(const String& url) const;

    private:
        static String canonicalize(const String&);
        
        static String decodeURL(const String& url, const TextEncoding& encoding = UTF8Encoding(), bool decodeHTMLentities = true);
        
        static String decodeHTMLEntities(const String&, bool leaveUndecodableHTMLEntitiesUntouched = true);

        bool findInRequest(const String&, bool decodeHTMLentities = true) const;

        bool findInRequest(Frame*, const String&, bool decodeHTMLentities = true) const;

        // The frame to audit.
        Frame* m_frame;
    };

} // namespace WebCore

#endif // XSSAuditor_h
