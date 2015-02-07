

#import <QuartzCore/CoreImage.h>

#if ENABLE(SVG) && ENABLE(SVG_FILTERS)

@interface WKDisplacementMapFilter : CIFilter {
    CIImage  *inputImage;
    CIImage  *inputDisplacementMap;
    CIVector *inputXChannelSelector;
    CIVector *inputYChannelSelector;
    NSNumber *inputScale;
}

@end

#endif // ENABLE(SVG) && ENABLE(SVG_FILTERS)
