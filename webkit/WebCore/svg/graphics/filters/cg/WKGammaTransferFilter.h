

#import <QuartzCore/CoreImage.h>

#if ENABLE(SVG) && ENABLE(SVG_FILTERS)

@interface WKGammaTransferFilter : CIFilter {
    CIImage  *inputImage;
    NSNumber *inputAmplitude;
    NSNumber *inputExponent;
    NSNumber *inputOffset;
}

@end

#endif // ENABLE(SVG) && ENABLE(SVG_FILTERS)
