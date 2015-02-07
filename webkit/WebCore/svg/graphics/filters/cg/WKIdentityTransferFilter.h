

#import <QuartzCore/CoreImage.h>

#if ENABLE(SVG) && ENABLE(SVG_FILTERS)

@interface WKIdentityTransferFilter : CIFilter {
    CIImage  *inputImage;
}

@end

#endif // ENABLE(SVG) && ENABLE(SVG_FILTERS)
