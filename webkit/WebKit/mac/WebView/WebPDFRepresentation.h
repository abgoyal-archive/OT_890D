

#import <Foundation/Foundation.h>

@protocol WebDocumentRepresentation;

@interface WebPDFRepresentation : NSObject <WebDocumentRepresentation>
+ (NSArray *)supportedMIMETypes;
@end
