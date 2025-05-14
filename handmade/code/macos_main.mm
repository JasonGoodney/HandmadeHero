#include <AppKit/AppKit.h>
#include <Carbon/Carbon.h>
#include <Foundation/Foundation.h>
#include <cstdint>
#include <cstdlib>
#include <stdio.h>

#define internal static
#define local static
#define global static

typedef uint8_t u8;
typedef uint16_t u16;
typedef uint32_t u32;
typedef uint64_t u64;
typedef int8_t s8;
typedef int16_t s16;
typedef int32_t s32;
typedef int64_t s64;
typedef float f32;
typedef double f64;

global const u16 RENDER_WIDTH = 1280;
global const u16 RENDER_HEIGHT = 720;
global const int BYTES_PER_PIXEL = 4;

global BOOL RUNNING;
global u8 *buffer;
global int bitmapWidth;
global int bitmapHeight;
global int pitch;
global int x_offset = 0;
global int y_offset = 0;

internal void macos_draw_buffer(NSWindow *window);
internal void macos_refresh_buffer(NSWindow *window);
internal void render_weird_gradient(int x_offset, int y_offset);

@interface HandmadeWindowDelegate : NSObject <NSWindowDelegate>
;
@end

@implementation HandmadeWindowDelegate
;
- (void)windowWillClose:(NSNotification *)notification {
  RUNNING = NO;
}
- (void)windowDidResize:(NSNotification *)notification {
  NSWindow *window = (NSWindow *)notification.object;
  macos_refresh_buffer(window);
  render_weird_gradient(x_offset, y_offset);
  macos_draw_buffer(window);
}
@end

int main(int argc, const char *argv[]) {

  NSRect screenRect = [[NSScreen mainScreen] frame];

  NSRect windowRect = NSMakeRect((screenRect.size.width - RENDER_WIDTH) * 0.5,
                                 (screenRect.size.height - RENDER_HEIGHT) * 0.5,
                                 RENDER_WIDTH, RENDER_HEIGHT);

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
  window.contentView.wantsLayer = YES;

  macos_refresh_buffer(window);

  x_offset = 0;
  y_offset = 0;
  RUNNING = true;
  while (RUNNING) {

    render_weird_gradient(x_offset, y_offset);
    macos_draw_buffer(window);
    x_offset += 1;
    y_offset += 1;

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

void macos_draw_buffer(NSWindow *window) {
  @autoreleasepool {
    NSBitmapImageRep *imageRep = [[[NSBitmapImageRep alloc]
        initWithBitmapDataPlanes:&buffer
                      pixelsWide:bitmapWidth
                      pixelsHigh:bitmapHeight
                   bitsPerSample:8
                 samplesPerPixel:BYTES_PER_PIXEL
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
}

void macos_refresh_buffer(NSWindow *window) {
  if (buffer) {
    free(buffer);
    buffer = NULL;
  }
  bitmapWidth = window.contentView.bounds.size.width;
  bitmapHeight = window.contentView.bounds.size.height;
  pitch = bitmapWidth * BYTES_PER_PIXEL;
  buffer = (u8 *)malloc(pitch * bitmapHeight);
}

void render_weird_gradient(int x_offset, int y_offset) {
  u8 *row = buffer;
  for (int y = 0; y < bitmapHeight; ++y) {
    u32 *pixel = (u32 *)row;
    for (int x = 0; x < bitmapWidth; ++x) {
      u8 r = 0;
      u8 g = (u8)(y + y_offset);
      u8 b = (u8)(x + x_offset);
      u8 a = 255;
      *pixel = (r | g << 8 | b << 16 | a << 24);
      pixel += 1;
    }
    row += pitch;
  }
}