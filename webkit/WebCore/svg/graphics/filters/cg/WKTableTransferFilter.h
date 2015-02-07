

#import <QuartzCore/CoreImage.h>

#if ENABLE(SVG) && ENABLE(SVG_FILTERS)

@interface WKTableTransferFilter : CIFilter {
    CIImage *inputImage;
    CIImage *inputTable;
    CIVector *inputSelector;
}

@end

#endif
