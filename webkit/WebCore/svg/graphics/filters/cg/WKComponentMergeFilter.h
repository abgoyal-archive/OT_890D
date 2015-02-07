

#import <QuartzCore/CoreImage.h>

#if ENABLE(SVG) && ENABLE(SVG_FILTERS)

@interface WKComponentMergeFilter : CIFilter {
    CIImage  *inputFuncR;
    CIImage  *inputFuncG;
    CIImage  *inputFuncB;
    CIImage  *inputFuncA;
}

@end

#endif // ENABLE(SVG) && ENABLE(SVG_FILTERS)
