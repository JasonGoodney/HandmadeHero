#include <AppKit/AppKit.h>
#include <AppKit/NSEvent.h>
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

typedef struct MacosOffscreenBuffer
{
    u8 *buffer;
    int width;
    int height;
    int pitch;
} Buffer;

typedef struct RectInt
{
    int x, y, width, height;
} RectInt;

global const u16 RENDER_WIDTH   = 1280;
global const u16 RENDER_HEIGHT  = 720;
global const u8 BYTES_PER_PIXEL = 4;

global BOOL RUNNING;
global Buffer global_backbuffer;
global int x_offset = 0;
global int y_offset = 0;

internal RectInt get_window_rect(const NSWindow *window);
internal void macos_buffer_resize(Buffer *buffer, int width, int height);
internal void macos_buffer_display(Buffer *buffer, const NSWindow *window);
internal void render_weird_gradient(const Buffer *buffer, int x_offset,
                                    int y_offset);

@interface HandmadeWindowDelegate : NSObject <NSWindowDelegate>
;
@end

@implementation HandmadeWindowDelegate
;
- (void)windowWillClose:(NSNotification *)notification
{
    RUNNING = NO;
}
- (void)windowDidResize:(NSNotification *)notification
{
    NSWindow *window = (NSWindow *)notification.object;
    RectInt rect     = get_window_rect(window);
    macos_buffer_resize(&global_backbuffer, rect.width, rect.height);
    render_weird_gradient(&global_backbuffer, x_offset, y_offset);
    macos_buffer_display(&global_backbuffer, window);

    NSString *title = [NSString
        stringWithFormat:@"Handmade Here (%dx%d)", rect.width, rect.height];
    [window setTitle:title];
}
@end

int main(int argc, const char *argv[])
{

    NSRect screenRect = [[NSScreen mainScreen] frame];

    NSRect windowRect =
        NSMakeRect((screenRect.size.width - RENDER_WIDTH) * 0.5,
                   (screenRect.size.height - RENDER_HEIGHT) * 0.5, RENDER_WIDTH,
                   RENDER_HEIGHT);

    NSWindow *window =
        [[NSWindow alloc] initWithContentRect:windowRect
                                    styleMask:NSWindowStyleMaskTitled |
                                              NSWindowStyleMaskClosable |
                                              NSWindowStyleMaskMiniaturizable |
                                              NSWindowStyleMaskResizable
                                      backing:NSBackingStoreBuffered
                                        defer:NO];
    [window setBackgroundColor:NSColor.blackColor];
    // [window makeKeyAndOrderFront:window];
    [window orderFrontRegardless];
    [window setAcceptsMouseMovedEvents:true];

    window.contentView.wantsLayer = YES;

    HandmadeWindowDelegate *windowDelegate =
        [[HandmadeWindowDelegate alloc] init];
    [window setDelegate:windowDelegate];

    RectInt rect = get_window_rect(window);
    macos_buffer_resize(&global_backbuffer, rect.width, rect.height);
    NSString *title = [NSString stringWithFormat:@"Handmade Here (%dx%d)",
                                                 global_backbuffer.width,
                                                 global_backbuffer.height];
    [window setTitle:title];

    x_offset = 0;
    y_offset = 0;
    RUNNING  = true;
    while (RUNNING)
    {

        render_weird_gradient(&global_backbuffer, x_offset, y_offset);
        macos_buffer_display(&global_backbuffer, window);

        NSEvent *event;
        do
        {
            event = [NSApp nextEventMatchingMask:NSEventMaskAny
                                       untilDate:nil
                                          inMode:NSDefaultRunLoopMode
                                         dequeue:YES];

            NSEventType type = [event type];

            if (type == NSEventTypeMouseEntered)
            {
                printf("mouse entered\n");
            }
            else if (type == NSEventTypeMouseMoved)
            {
                printf("mouse position: %f.0, %f.0\n", event.locationInWindow.x,
                       event.locationInWindow.y);
            }
            else if (type == NSEventTypeMouseExited)
            {
                printf("mouse exited\n");
            }
            else if (type == NSEventTypeKeyDown)
            {
                printf("keyCode: %i\n", event.keyCode);
            }
            else
            {
                [NSApp sendEvent:event];
            }
        } while (event != nil);
    }

    printf("Handmade Hero finished running.\n");

    return 0;
}

internal RectInt get_window_rect(const NSWindow *window)
{
    RectInt rect;
    rect.width  = window.contentView.bounds.size.width;
    rect.height = window.contentView.bounds.size.height;
    return rect;
}

void macos_buffer_display(Buffer *buffer, const NSWindow *window)
{
    @autoreleasepool
    {
        NSBitmapImageRep *imageRep = [[[NSBitmapImageRep alloc]
            initWithBitmapDataPlanes:&buffer->buffer
                          pixelsWide:buffer->width
                          pixelsHigh:buffer->height
                       bitsPerSample:8
                     samplesPerPixel:BYTES_PER_PIXEL
                            hasAlpha:YES
                            isPlanar:NO
                      colorSpaceName:NSDeviceRGBColorSpace
                         bytesPerRow:buffer->pitch
                        bitsPerPixel:32] autorelease];

        NSSize imageSize = NSMakeSize(buffer->width, buffer->height);
        NSImage *image = [[[NSImage alloc] initWithSize:imageSize] autorelease];
        [image addRepresentation:imageRep];

        window.contentView.layer.contents = image;
    }
}

void macos_buffer_resize(Buffer *buffer, int width, int height)
{
    if (buffer->buffer)
    {
        free(buffer->buffer);
        buffer->buffer = NULL;
    }

    buffer->width  = width;
    buffer->height = height;
    buffer->pitch  = width * BYTES_PER_PIXEL;
    buffer->buffer = (u8 *)malloc(buffer->pitch * height);
}

void render_weird_gradient(const Buffer *buffer, int x_offset, int y_offset)
{
    u8 *row = buffer->buffer;
    for (int y = 0; y < buffer->height; ++y)
    {
        u32 *pixel = (u32 *)row;
        int dy     = y - y_offset;
        for (int x = 0; x < buffer->width; ++x)
        {
            int dx = x - x_offset;
            u8 r   = 0;
            u8 g   = (u8)(y);
            u8 b   = (u8)(x);
            u8 a   = 255;
            *pixel = (r | g << 8 | b << 16 | a << 24);
            pixel += 1;
        }
        row += buffer->pitch;
    }
}