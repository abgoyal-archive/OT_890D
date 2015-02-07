

#ifndef SVGResourceFilterPlatformDataMac_h
#define SVGResourceFilterPlatformDataMac_h

#if ENABLE(SVG) && ENABLE(SVG_FILTERS)

#include "SVGResourceFilter.h"

#include <ApplicationServices/ApplicationServices.h>
#include <wtf/RetainPtr.h>

@class CIImage;
@class CIFilter;
@class CIContext;
@class NSArray;
@class NSMutableDictionary;

namespace WebCore {
    class SVGResourceFilterPlatformDataMac : public SVGResourceFilterPlatformData {
    public:
        SVGResourceFilterPlatformDataMac(SVGResourceFilter*);
        virtual ~SVGResourceFilterPlatformDataMac();
        
        CIImage* imageForName(const String&) const;
        void setImageForName(CIImage*, const String&);
        
        void setOutputImage(const SVGFilterEffect*, CIImage*);
        CIImage* inputImage(const SVGFilterEffect*);
        
        NSArray* getCIFilterStack(CIImage* inputImage, const FloatRect& bbox);
        
        RetainPtr<CIContext> m_filterCIContext;
        CGLayerRef m_filterCGLayer;
        RetainPtr<NSMutableDictionary> m_imagesByName;
        SVGResourceFilter* m_filter;
    };
}

#endif // #if ENABLE(SVG) && ENABLE(SVG_FILTERS)

#endif // SVGResourceFilterPlatformDataMac_h
