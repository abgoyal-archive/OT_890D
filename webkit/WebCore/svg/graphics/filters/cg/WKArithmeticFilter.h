

#import <QuartzCore/CoreImage.h>

#if ENABLE(SVG) && ENABLE(SVG_FILTERS)

@interface WKArithmeticFilter : CIFilter {
    CIImage  *inputImage;
    CIImage  *inputBackgroundImage;
    NSNumber *inputK1;
    NSNumber *inputK2;
    NSNumber *inputK3;
    NSNumber *inputK4;
}

@end

#endif // ENABLE(SVG) && ENABLE(SVG_FILTERS)
