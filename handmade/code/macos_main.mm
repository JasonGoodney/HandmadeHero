#include <AppKit/AppKit.h>
#include <Foundation/Foundation.h>
#include <cstdint>
#include <cstdlib>
#include <stdio.h>

#define internal static
#define local_persist static
#define global_variable static

global_variable int GlobalRenderingWidth = 1280;
global_variable int GlobalRenderingHeight = 720;
global_variable BOOL Running;

@interface HandmadeWindowDelegate : NSObject <NSWindowDelegate>
;
@end

@implementation HandmadeWindowDelegate
;
- (void)windowWillClose:(NSNotification *)notification {
  Running = NO;
}
@end

int main(int argc, const char *argv[]) {

  NSRect screenRect = [[NSScreen mainScreen] frame];

  NSRect windowRect =
      NSMakeRect((screenRect.size.width - GlobalRenderingWidth) * 0.5,
                 (screenRect.size.height - GlobalRenderingHeight) * 0.5,
                 GlobalRenderingWidth, GlobalRenderingHeight);

  NSWindow *window = [[NSWindow alloc]
      initWithContentRect:windowRect
                styleMask:NSWindowStyleMaskTitled | NSWindowStyleMaskClosable |
                          NSWindowStyleMaskMiniaturizable |
                          NSWindowStyleMaskResizable
                  backing:NSBackingStoreBuffered
                    defer:NO];
  [window setBackgroundColor:NSColor.blackColor];
  [window setTitle:@"Handmade Hero"];
  [window makeKeyAndOrderFront:nil];

  HandmadeWindowDelegate *windowDelegate =
      [[HandmadeWindowDelegate alloc] init];
  [window setDelegate:windowDelegate];

  // pitch
  int bitmapWidth = window.contentView.bounds.size.width;
  int bitmapHeight = window.contentView.bounds.size.height;
  int bytesPerPixel = 4;

  int pitch = bitmapWidth * bytesPerPixel;

  uint8_t *buffer = (uint8_t *)malloc(pitch * bitmapHeight);

  Running = true;
  while (Running) {

    @autoreleasepool {
      NSBitmapImageRep *imageRep = [[[NSBitmapImageRep alloc]
          initWithBitmapDataPlanes:&buffer
                        pixelsWide:bitmapWidth
                        pixelsHigh:bitmapHeight
                     bitsPerSample:8
                   samplesPerPixel:bytesPerPixel
                          hasAlpha:YES
                          isPlanar:NO
                    colorSpaceName:NSDeviceRGBColorSpace
                       bytesPerRow:pitch
                      bitsPerPixel:32] autorelease];
      NSSize imageSize = NSMakeSize(bitmapWidth, bitmapHeight);
      NSImage *image = [[[NSImage alloc] initWithSize:imageSize] autorelease];
      [image addRepresentation:imageRep];

      window.contentView.layer.contents = image;
    }

    NSEvent *event;
    do {
      event = [NSApp nextEventMatchingMask:NSEventMaskAny
                                 untilDate:nil
                                    inMode:NSDefaultRunLoopMode
                                   dequeue:YES];

      switch ([event type]) {
      default:
        [NSApp sendEvent:event];
      }
    } while (event != nil);
  }

  printf("Handmade Hero finished running.\n");

  return 0;
}
