

#import <QuartzCore/CoreImage.h>

#if ENABLE(SVG) && ENABLE(SVG_FILTERS)

@interface WKNormalMapFilter : CIFilter {
    CIImage *inputImage;
    NSNumber *inputSurfaceScale;
}
@end

#endif // ENABLE(SVG) && ENABLE(SVG_FILTERS)
