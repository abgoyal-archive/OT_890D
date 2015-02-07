

#import <QuartzCore/CoreImage.h>

#if ENABLE(SVG) && ENABLE(SVG_FILTERS)

@interface WKSpotLightFilter : CIFilter {
    CIImage  *inputLightVectors;
    CIVector *inputLightDirection;
    NSNumber *inputSpecularExponent;
    NSNumber *inputLimitingConeAngle;
}
@end

#endif // ENABLE(SVG) && ENABLE(SVG_FILTERS)
