

#import <QuartzCore/CoreImage.h>

#if ENABLE(SVG) && ENABLE(SVG_FILTERS)

@interface WKLinearTransferFilter : CIFilter {
    CIImage  *inputImage;
    NSNumber *inputSlope;
    NSNumber *inputIntercept;
}

@end

#endif // ENABLE(SVG) && ENABLE(SVG_FILTERS)
