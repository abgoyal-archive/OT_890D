

#import <Foundation/Foundation.h>

#import <WebKit/WebFrame.h>

@interface WebCoreStatistics : NSObject
{
}

+ (NSArray *)statistics;

+ (size_t)javaScriptObjectsCount;
+ (size_t)javaScriptGlobalObjectsCount;
+ (size_t)javaScriptProtectedObjectsCount;
+ (size_t)javaScriptProtectedGlobalObjectsCount;
+ (NSCountedSet *)javaScriptProtectedObjectTypeCounts;

+ (void)garbageCollectJavaScriptObjects;
+ (void)garbageCollectJavaScriptObjectsOnAlternateThreadForDebugging:(BOOL)waitUntilDone;

+ (size_t)iconPageURLMappingCount;
+ (size_t)iconRetainedPageURLCount;
+ (size_t)iconRecordCount;
+ (size_t)iconsWithDataCount;

+ (size_t)cachedFontDataCount;
+ (size_t)cachedFontDataInactiveCount;
+ (void)purgeInactiveFontData;
+ (size_t)glyphPageCount;

+ (BOOL)shouldPrintExceptions;
+ (void)setShouldPrintExceptions:(BOOL)print;

+ (void)startIgnoringWebCoreNodeLeaks;
+ (void)stopIgnoringWebCoreNodeLeaks;

+ (NSDictionary *)memoryStatistics;
+ (void)returnFreeMemoryToSystem;

+ (int)cachedPageCount;
+ (int)cachedFrameCount;
+ (int)autoreleasedPageCount;

// Deprecated, but used by older versions of Safari.
+ (void)emptyCache;
+ (void)setCacheDisabled:(BOOL)disabled;
+ (size_t)javaScriptNoGCAllowedObjectsCount;
+ (size_t)javaScriptReferencedObjectsCount;
+ (NSSet *)javaScriptRootObjectClasses;
+ (NSCountedSet *)javaScriptRootObjectTypeCounts;
+ (size_t)javaScriptInterpretersCount;

@end

@interface WebFrame (WebKitDebug)
- (NSString *)renderTreeAsExternalRepresentation;
@end
