

#import <QuartzCore/CoreImage.h>

#if ENABLE(SVG) && ENABLE(SVG_FILTERS)

@interface WKSpecularLightingFilter : CIFilter {
    CISampler *inputNormalMap;
    CISampler *inputLightVectors;
    CIColor  *inputLightingColor;
    NSNumber *inputSurfaceScale;
    NSNumber *inputSpecularConstant;
    NSNumber *inputSpecularExponent;
    NSNumber *inputKernelUnitLengthX;
    NSNumber *inputKernelUnitLengthY;
}
@end

#endif // ENABLE(SVG) && ENABLE(SVG_FILTERS)
