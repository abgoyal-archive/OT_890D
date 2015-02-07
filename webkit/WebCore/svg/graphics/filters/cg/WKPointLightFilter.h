

#import <QuartzCore/CoreImage.h>

#if ENABLE(SVG) && ENABLE(SVG_FILTERS)

@interface WKPointLightFilter : CIFilter {
    CIImage  *inputNormalMap;
    CIVector *inputLightPosition;
    NSNumber *inputSurfaceScale;
}

@end

#endif // ENABLE(SVG) && ENABLE(SVG_FILTERS)
