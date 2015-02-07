

#import <QuartzCore/CoreImage.h>

#if ENABLE(SVG) && ENABLE(SVG_FILTERS)

@interface WKDiscreteTransferFilter : CIFilter {
    CIImage  *inputImage;
    CIImage  *inputTable;
    CIVector *inputSelector;
}

@end

#endif // ENABLE(SVG) && ENABLE(SVG_FILTERS)
