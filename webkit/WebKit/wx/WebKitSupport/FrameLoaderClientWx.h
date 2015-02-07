
 
#ifndef FrameLoaderClientWx_H
#define FrameLoaderClientWx_H

#include "FrameLoaderClient.h"
#include "FrameLoader.h"
#include "KURL.h"
#include "ResourceResponse.h"

class wxWebView;

namespace WebCore {

    class AuthenticationChallenge;
    class DocumentLoader;
    class Element;
    class FormState;
    class NavigationAction;
    class String;
    class ResourceLoader;

    struct LoadErrorResetToken;

    class FrameLoaderClientWx : public FrameLoaderClient {
    public:
        FrameLoaderClientWx();
        ~FrameLoaderClientWx();
        void setFrame(Frame *frame);
        void setWebView(wxWebView *webview);
        virtual void detachFrameLoader();

        virtual bool hasWebView() const; // mainly for assertions

        virtual bool hasBackForwardList() const;
        virtual void resetBackForwardList();

        virtual bool provisionalItemIsTarget() const;

        virtual void makeRepresentation(DocumentLoader*);
        virtual void forceLayout();
        virtual void forceLayoutForNonHTML();

        virtual void updateHistoryForCommit();

        virtual void updateHistoryForBackForwardNavigation();
        virtual void updateHistoryForReload();
        virtual void updateHistoryForStandardLoad();
        virtual void updateHistoryForInternalLoad();

        virtual void updateHistoryAfterClientRedirect();

        virtual void setCopiesOnScroll();

        virtual LoadErrorResetToken* tokenForLoadErrorReset();
        virtual void resetAfterLoadError(LoadErrorResetToken*);
        virtual void doNotResetAfterLoadError(LoadErrorResetToken*);

        virtual void willCloseDocument();

        virtual void detachedFromParent2();
        virtual void detachedFromParent3();

        virtual void frameLoaderDestroyed();
        virtual bool canHandleRequest(const ResourceRequest&) const;

        virtual void dispatchDidHandleOnloadEvents();
        virtual void dispatchDidReceiveServerRedirectForProvisionalLoad();
        virtual void dispatchDidCancelClientRedirect();
        virtual void dispatchWillPerformClientRedirect(const KURL&, double interval, double fireDate);
        virtual void dispatchDidChangeLocationWithinPage();
        virtual void dispatchWillClose();
        virtual void dispatchDidReceiveIcon();
        virtual void dispatchDidStartProvisionalLoad();
        virtual void dispatchDidReceiveTitle(const String& title);
        virtual void dispatchDidCommitLoad();
        virtual void dispatchDidFinishDocumentLoad();
        virtual void dispatchDidFinishLoad();
        virtual void dispatchDidFirstLayout();
        virtual void dispatchDidFirstVisuallyNonEmptyLayout();

        virtual void dispatchShow();
        virtual void cancelPolicyCheck();

        virtual void dispatchWillSubmitForm(FramePolicyFunction, PassRefPtr<FormState>);

        virtual void dispatchDidLoadMainResource(DocumentLoader*);
        virtual void revertToProvisionalState(DocumentLoader*);

        virtual void postProgressStartedNotification();
        virtual void postProgressEstimateChangedNotification();
        virtual void postProgressFinishedNotification();

        virtual void progressStarted();
        virtual void progressCompleted();
        virtual void setMainFrameDocumentReady(bool);
        virtual void willChangeTitle(DocumentLoader*);
        virtual void didChangeTitle(DocumentLoader*);
        virtual void finishedLoading(DocumentLoader*);

        virtual bool canShowMIMEType(const String& MIMEType) const;
        virtual bool representationExistsForURLScheme(const String& URLScheme) const;
        virtual String generatedMIMETypeForURLScheme(const String& URLScheme) const;

        virtual void frameLoadCompleted();
        virtual void saveViewStateToItem(HistoryItem*);
        virtual void restoreViewState();
        virtual void restoreScrollPositionAndViewState();
        virtual void provisionalLoadStarted();
        virtual bool shouldTreatURLAsSameAsCurrent(const KURL&) const;
        virtual void addHistoryItemForFragmentScroll();
        virtual void didFinishLoad();
        virtual void prepareForDataSourceReplacement();
        virtual void setTitle(const String& title, const KURL&);

        virtual String userAgent(const KURL&);

        virtual void savePlatformDataToCachedFrame(WebCore::CachedFrame*);
        virtual void transitionToCommittedFromCachedFrame(WebCore::CachedFrame*);
        virtual void transitionToCommittedForNewPage();
        
        virtual void updateGlobalHistory();
        virtual void updateGlobalHistoryRedirectLinks();
        virtual bool shouldGoToHistoryItem(HistoryItem*) const;
        virtual void saveScrollPositionAndViewStateToItem(HistoryItem*);
        virtual bool canCachePage() const;
        
        virtual void setMainDocumentError(DocumentLoader*, const ResourceError&);
        virtual void committedLoad(DocumentLoader*, const char*, int);
        virtual ResourceError cancelledError(const ResourceRequest&);
        virtual ResourceError blockedError(const ResourceRequest&);
        virtual ResourceError cannotShowURLError(const ResourceRequest&);
        virtual ResourceError interruptForPolicyChangeError(const ResourceRequest&);
        virtual ResourceError cannotShowMIMETypeError(const ResourceResponse&);
        virtual ResourceError fileDoesNotExistError(const ResourceResponse&);
        virtual bool shouldFallBack(const ResourceError&);
        virtual WTF::PassRefPtr<DocumentLoader> createDocumentLoader(const ResourceRequest&, const SubstituteData&);
        virtual void download(ResourceHandle*, const ResourceRequest&, const ResourceRequest&, const ResourceResponse&);

        virtual void assignIdentifierToInitialRequest(unsigned long identifier, DocumentLoader*, const ResourceRequest&);
        
        virtual void dispatchWillSendRequest(DocumentLoader*, unsigned long, ResourceRequest&, const ResourceResponse&);
        virtual bool shouldUseCredentialStorage(DocumentLoader*, unsigned long identifier);
        virtual void dispatchDidReceiveAuthenticationChallenge(DocumentLoader*, unsigned long identifier, const AuthenticationChallenge&);
        virtual void dispatchDidCancelAuthenticationChallenge(DocumentLoader*, unsigned long identifier, const AuthenticationChallenge&);
        virtual void dispatchDidReceiveResponse(DocumentLoader*, unsigned long, const ResourceResponse&);
        virtual void dispatchDidReceiveContentLength(DocumentLoader*, unsigned long, int);
        virtual void dispatchDidFinishLoading(DocumentLoader*, unsigned long);
        virtual void dispatchDidFailLoading(DocumentLoader*, unsigned long, const ResourceError&);
        virtual bool dispatchDidLoadResourceFromMemoryCache(DocumentLoader*, const ResourceRequest&, const ResourceResponse&, int);
        virtual void dispatchDidLoadResourceByXMLHttpRequest(unsigned long, const ScriptString&);

        virtual void dispatchDidFailProvisionalLoad(const ResourceError&);
        virtual void dispatchDidFailLoad(const ResourceError&);
        virtual Frame* dispatchCreatePage();
        virtual void dispatchDecidePolicyForMIMEType(FramePolicyFunction function, const String&, const ResourceRequest&);
        virtual void dispatchDecidePolicyForNewWindowAction(FramePolicyFunction function, const NavigationAction&, const ResourceRequest&, PassRefPtr<FormState>, const String&);
        virtual void dispatchDecidePolicyForNavigationAction(FramePolicyFunction function, const NavigationAction&, const ResourceRequest&, PassRefPtr<FormState>);
        virtual void dispatchUnableToImplementPolicy(const ResourceError&);

        virtual void startDownload(const ResourceRequest&);
        
        // FIXME: This should probably not be here, but it's needed for the tests currently
        virtual void partClearedInBegin();

        virtual PassRefPtr<Frame> createFrame(const KURL& url, const String& name, HTMLFrameOwnerElement* ownerElement,
                                   const String& referrer, bool allowsScrolling, int marginWidth, int marginHeight);
        virtual PassRefPtr<Widget> createPlugin(const IntSize&, HTMLPlugInElement*, const KURL&, const Vector<String>&, const Vector<String>&, const String&, bool loadManually) ;
        virtual void redirectDataToPlugin(Widget* pluginWidget);
        virtual ResourceError pluginWillHandleLoadError(const ResourceResponse&);
        
        virtual PassRefPtr<Widget> createJavaAppletWidget(const IntSize&, HTMLAppletElement*, const KURL& baseURL, const Vector<String>& paramNames, const Vector<String>& paramValues);

        virtual ObjectContentType objectContentType(const KURL& url, const String& mimeType);
        virtual String overrideMediaType() const;

        virtual void windowObjectCleared();
        virtual void documentElementAvailable();
        
        virtual void didPerformFirstNavigation() const;
        
        virtual void registerForIconNotification(bool listen = true);

    private:
        Frame *m_frame;
        wxWebView *m_webView;
        ResourceResponse m_response;
        bool m_firstData;
    };

}

#endif