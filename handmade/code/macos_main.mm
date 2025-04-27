#include <AppKit/AppKit.h>
#include <stdio.h>

static int GlobalRenderingWidth = 1280;
static int GlobalRenderingHeight = 720;

static BOOL Running = YES;

@interface HandmadeWindowDelegate: NSObject<NSWindowDelegate>;
@end

@implementation HandmadeWindowDelegate;
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
  [window setBackgroundColor:NSColor.redColor];
  [window setTitle:@"Handmade Hero"];
  [window makeKeyAndOrderFront:nil];

  HandmadeWindowDelegate *windowDelegate = [[HandmadeWindowDelegate alloc] init];
  [window setDelegate:windowDelegate];

  while (Running) {
    NSEvent *event;
    do {
       event = [NSApp nextEventMatchingMask:NSEventMaskAny
                                        untilDate:nil
                                           inMode:NSDefaultRunLoopMode
                                          dequeue:YES];

      switch ([event type]) {
      default:
        [NSApp sendEvent: event];
      }
    } while (event != nil);
  }

  printf("Handmade Hero finished running.\n");

  return 0;
}
