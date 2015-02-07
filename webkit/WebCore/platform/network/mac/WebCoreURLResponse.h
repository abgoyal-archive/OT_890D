

@interface NSURLResponse (WebCoreURLResponse)
-(NSString *)_webcore_reportedMIMEType;
@end

void swizzleMIMETypeMethodIfNecessary();
