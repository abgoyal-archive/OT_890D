

#import <Cocoa/Cocoa.h>

@interface SVGTest : NSObject {
    NSString *_svgPath;
    NSString *_imagePath;
    
    NSImage *_image;
    NSImage *_compositeImage;
    BOOL _hasPassed;
}

+ (id)testWithSVGPath:(NSString *)svgPath imagePath:(NSString *)imagePath;
- (id)initWithSVGPath:(NSString *)svgPath imagePath:(NSString *)imagePath;

- (NSString *)imagePath;
- (NSString *)svgPath;

- (NSImage *)image;
- (NSImage *)compositeImage;
- (NSString *)name;

@end
