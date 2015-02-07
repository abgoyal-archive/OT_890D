

#ifndef SVGResourceClipper_h
#define SVGResourceClipper_h

#if ENABLE(SVG)

#include "SVGResource.h"
#include "Path.h"

namespace WebCore {

    struct ClipData {
        Path path;
        WindRule windRule;
        bool bboxUnits : 1;
    };

    class ClipDataList { 
    public:
        void addPath(const Path& pathData, WindRule windRule, bool bboxUnits)
        {
            ClipData clipData;
            
            clipData.path = pathData;
            clipData.windRule = windRule;
            clipData.bboxUnits = bboxUnits;
            
            m_clipData.append(clipData);
        }
        
        void clear() { m_clipData.clear(); }
        const Vector<ClipData>& clipData() const { return m_clipData; }
        bool isEmpty() const { return m_clipData.isEmpty(); }
    private:
        Vector<ClipData> m_clipData;
    };  

    class GraphicsContext;

    class SVGResourceClipper : public SVGResource {
    public:
        static PassRefPtr<SVGResourceClipper> create() { return adoptRef(new SVGResourceClipper); }
        virtual ~SVGResourceClipper();
      
        void resetClipData();
        void addClipData(const Path&, WindRule, bool bboxUnits);

        const ClipDataList& clipData() const;
        
        virtual SVGResourceType resourceType() const { return ClipperResourceType; }
        virtual TextStream& externalRepresentation(TextStream&) const;

        // To be implemented by the specific rendering devices
        void applyClip(GraphicsContext*, const FloatRect& boundingBox) const;
    private:
        SVGResourceClipper();
        ClipDataList m_clipData;
    };

    TextStream& operator<<(TextStream&, WindRule);
    TextStream& operator<<(TextStream&, const ClipData&);

    SVGResourceClipper* getClipperById(Document*, const AtomicString&);

} // namespace WebCore

#endif

#endif // SVGResourceClipper_h
