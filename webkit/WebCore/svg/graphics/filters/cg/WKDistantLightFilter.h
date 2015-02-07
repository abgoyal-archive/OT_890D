

#import <QuartzCore/CoreImage.h>

#if ENABLE(SVG) && ENABLE(SVG_FILTERS)

@interface WKDistantLightFilter : CIFilter {
    CIImage  * inputNormalMap;
    CIVector * inputLightDirection;
}
@end

#endif // ENABLE(SVG) && ENABLE(SVG_FILTERS)
