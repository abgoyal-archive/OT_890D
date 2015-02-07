

#ifndef CFDictionaryPropertyBag_H
#define CFDictionaryPropertyBag_H

#include <CoreFoundation/CoreFoundation.h>
#include <wtf/RetainPtr.h>

class CFDictionaryPropertyBag : public IPropertyBag
{
public:
    static CFDictionaryPropertyBag* createInstance();
protected:
    CFDictionaryPropertyBag();
    ~CFDictionaryPropertyBag();

public:
    // IUnknown
    virtual HRESULT STDMETHODCALLTYPE QueryInterface(REFIID riid, void** ppvObject);
    virtual ULONG STDMETHODCALLTYPE AddRef(void);
    virtual ULONG STDMETHODCALLTYPE Release(void);

    // IPropertyBag
    virtual /* [local] */ HRESULT STDMETHODCALLTYPE Read( 
        /* [in] */ LPCOLESTR pszPropName,
        /* [out][in] */ VARIANT *pVar,
        /* [in] */ IErrorLog *pErrorLog);
        
    virtual HRESULT STDMETHODCALLTYPE Write( 
        /* [in] */ LPCOLESTR pszPropName,
        /* [in] */ VARIANT *pVar);

    void setDictionary(CFMutableDictionaryRef dictionary);
    CFMutableDictionaryRef dictionary() const;

private:
    RetainPtr<CFMutableDictionaryRef> m_dictionary;
    ULONG m_refCount;
};

#endif