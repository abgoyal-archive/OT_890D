

#import <QuartzCore/CoreImage.h>

#if ENABLE(SVG) && ENABLE(SVG_FILTERS)

@interface WKDiffuseLightingFilter : CIFilter {
    CISampler *inputNormalMap;
    CISampler *inputLightVectors;
    CIColor   *inputLightingColor;
    NSNumber  *inputSurfaceScale;
    NSNumber  *inputDiffuseConstant;
    NSNumber  *inputKernelUnitLengthX;
    NSNumber  *inputKernelUnitLengthY;
}
@end

#endif // ENABLE(SVG) && ENABLE(SVG_FILTERS)
