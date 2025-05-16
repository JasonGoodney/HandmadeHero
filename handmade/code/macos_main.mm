#include <AppKit/AppKit.h>
#include <AppKit/NSEvent.h>
#include <Carbon/Carbon.h>
#include <CoreFoundation/CoreFoundation.h>
#include <Foundation/Foundation.h>
#include <IOKit/IOReturn.h>
#include <IOKit/hid/IOHIDBase.h>
#include <IOKit/hid/IOHIDDevice.h>
#include <IOKit/hid/IOHIDDeviceTypes.h>
#include <IOKit/hid/IOHIDElement.h>
#include <IOKit/hid/IOHIDLib.h>
#include <IOKit/hid/IOHIDManager.h>
#include <IOKit/hid/IOHIDUsageTables.h>
#include <IOKit/hid/IOHIDValue.h>
#include <cstddef>
#include <cstdint>
#include <cstdio>
#include <cstdlib>
#include <objc/NSObjCRuntime.h>
#include <objc/objc.h>
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

enum
{
    HID_Page_GenericDesktop = 0x01,
    HID_Page_Button         = 0x09,
    HID_Page_Haptics        = 0x0E,
};

enum HatSwitchDirection
{
    HatSwitch_Top         = 0,
    HatSwitch_TopRight    = 1,
    HatSwitch_Right       = 2,
    HatSwitch_BottomRight = 3,
    HatSwitch_Bottom      = 4,
    HatSwitch_BottomLeft  = 5,
    HatSwitch_Left        = 6,
    HatSwitch_TopLeft     = 7,
    HatSwitch_None        = 8
};

typedef struct MacosOffscreenBuffer
{
    u8 *buffer;
    int width;
    int height;
    int pitch;
} Buffer;

struct DeviceUsage
{
    u32 usage_id;
    u32 state;
};

struct Gamepad
{
    struct DeviceUsage face_top, face_bottom, face_left, face_right;
    struct DeviceUsage dpad_x, dpad_y;
    struct DeviceUsage should_left, should_right;
    struct DeviceUsage trigger_left, trigger_right;
    struct DeviceUsage analog_stick_left, analog_stick_right;
};

typedef struct RectInt
{
    int x, y, width, height;
} RectInt;

global const u16 RENDER_WIDTH   = 64 * 12;
global const u16 RENDER_HEIGHT  = 64 * 8;
global const u8 BYTES_PER_PIXEL = 4;

global BOOL RUNNING;
global Buffer global_backbuffer;
global int x_offset       = 0;
global int y_offset       = 0;
global struct RectInt box = {RENDER_WIDTH / 2, RENDER_HEIGHT / 2, 50, 50};

internal RectInt macos_get_window_rect(const NSWindow *window);
internal void macos_buffer_resize(Buffer *buffer, int width, int height);
internal void macos_buffer_display(Buffer *buffer, const NSWindow *window);
internal void macos_init_gamepad(struct Gamepad *gamepad);
internal void macos_device_input_callback(void *context, IOReturn result,
                                          void *sender, IOHIDValueRef value);
internal void macos_device_callback(void *context, IOReturn result,
                                    void *sender, IOHIDDeviceRef device);
internal CFDictionaryRef macos_device_matching_dictionary(u32 usagePage,
                                                          u32 usage);

internal void render_weird_gradient(const Buffer *buffer, int x_offset,
                                    int y_offset);
internal void render_box(const RectInt *box, const Buffer *buffer, int x_offset,
                         int y_offset);
internal void render(const Buffer *buffer)
{
    // render_weird_gradient(buffer, x_offset, y_offset);
    render_box(&box, buffer, x_offset, y_offset);
}

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
    RectInt rect     = macos_get_window_rect(window);
    macos_buffer_resize(&global_backbuffer, rect.width, rect.height);
    // render_weird_gradient(&global_backbuffer, x_offset, y_offset);
    render(&global_backbuffer);
    macos_buffer_display(&global_backbuffer, window);

    NSString *title = [NSString
        stringWithFormat:@"Handmade Here (%dx%d)", rect.width, rect.height];
    [window setTitle:title];
}
@end

int main(int argc, const char *argv[])
{
    struct Gamepad gamepad = {};
    macos_init_gamepad(&gamepad);

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

    RectInt rect = macos_get_window_rect(window);
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

        if (gamepad.face_right.state)
        {
            box.x += 1;
        }
        else if (gamepad.face_left.state)
        {
            box.x -= 1;
        }
        else if (gamepad.face_top.state)
        {
            box.y -= 1;
        }
        else if (gamepad.face_bottom.state)
        {
            box.y += 1;
        }

        box.x += gamepad.dpad_x.state;
        box.y += gamepad.dpad_y.state;

        render(&global_backbuffer);
        macos_buffer_display(&global_backbuffer, window);

        NSEvent *event;
        do
        {
            event = [NSApp nextEventMatchingMask:NSEventMaskAny
                                       untilDate:nil
                                          inMode:NSDefaultRunLoopMode
                                         dequeue:YES];

            switch ([event type])
            {
            default:
                [NSApp sendEvent:event];
            }
        } while (event != nil);
    }

    printf("Handmade Hero finished running.\n");

    return 0;
}

internal RectInt macos_get_window_rect(const NSWindow *window)
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
        for (int x = 0; x < buffer->width; ++x)
        {
            u8 r   = 0;
            u8 g   = (u8)(y + y_offset);
            u8 b   = (u8)(x + x_offset);
            u8 a   = 255;
            *pixel = (r | g << 8 | b << 16 | a << 24);
            pixel += 1;
        }
        row += buffer->pitch;
    }
}

void render_box(const RectInt *box, const Buffer *buffer, int x_offset,
                int y_offset)
{

    int size = box->width;

    u8 *row = buffer->buffer;
    for (int y = 0; y < buffer->height; y += 1)
    {
        u32 *pixel = (u32 *)row;
        for (int x = 0; x < buffer->width; x += 1)
        {
            u8 r = 0;
            u8 g = 0;
            u8 b = 0;
            u8 a = 255;

            if (x >= box->x - size && x <= box->x + size &&
                y >= box->y - size && y <= box->y + size)
            {
                r = 255;
            }

            *pixel = (r | g << 8 | b << 16 | a << 24);
            pixel += 1;
        }
        row += buffer->pitch;
    }
}

internal CFDictionaryRef macos_device_matching_dictionary(u32 usagePage,
                                                          u32 usage)
{
    CFMutableDictionaryRef ref = CFDictionaryCreateMutable(
        kCFAllocatorDefault, 0, &kCFTypeDictionaryKeyCallBacks,
        &kCFTypeDictionaryValueCallBacks);

    if (ref == NULL)
    {
        return NULL;
    }

    CFNumberRef usagePageRef =
        CFNumberCreate(kCFAllocatorDefault, kCFNumberIntType, &usagePage);
    if (usagePageRef == NULL)
    {
        CFRelease(ref);
        return NULL;
    }

    CFDictionarySetValue(ref, CFSTR(kIOHIDDeviceUsagePageKey), usagePageRef);
    CFRelease(usagePageRef);

    CFNumberRef usageRef =
        CFNumberCreate(kCFAllocatorDefault, kCFNumberIntType, &usage);
    if (usageRef == NULL)
    {
        CFRelease(ref);
        return NULL;
    }

    CFDictionarySetValue(ref, CFSTR(kIOHIDDeviceUsageKey), usageRef);
    CFRelease(usageRef);

    return ref;
}

internal void macos_device_input_callback(void *context, IOReturn result,
                                          void *sender, IOHIDValueRef value)
{
    if (result != kIOReturnSuccess)
    {
        printf("Error device input callback:\n");
        return;
    }

    IOHIDElementRef element = IOHIDValueGetElement(value);
    u32 usagePage           = IOHIDElementGetUsagePage(element);
    u32 usage               = IOHIDElementGetUsage(element);
    s32 state               = (s32)IOHIDValueGetIntegerValue(value);

    if (!(usagePage == 0x01 && (usage >= 0x30 && usage <= 0x35)))
    {
        // ignoring output when Dualshock 4 is connected
        printf("Usage page: %x, usage: %x, state: %d\n ", usagePage, usage,
               state);
    }

    struct Gamepad *gamepad = (struct Gamepad *)context;

    if (usagePage == HID_Page_Button)
    {
        if (usage == gamepad->face_bottom.usage_id)
        {
            gamepad->face_bottom.state = state;
        }
        else if (usage == gamepad->face_top.usage_id)
        {
            gamepad->face_top.state = state;
        }
        else if (usage == gamepad->face_right.usage_id)
        {
            gamepad->face_right.state = state;
        }
        else if (usage == gamepad->face_left.usage_id)
        {
            gamepad->face_left.state = state;
        }
    }
    else if (usagePage == HID_Page_GenericDesktop)
    {
        // s64 analog =
        //     IOHIDValueGetScaledValue(value, kIOHIDValueScaleTypeCalibrated);
        // printf("analog, %ld\n", analog);
        if (usage == kHIDUsage_GD_Hatswitch)
        {
            switch (state)
            {
            case HatSwitch_Top:
                gamepad->dpad_x.state = 0;
                gamepad->dpad_y.state = -1;
                break;
            case HatSwitch_TopRight:
                gamepad->dpad_x.state = 1;
                gamepad->dpad_y.state = -1;
                break;
            case HatSwitch_Right:
                gamepad->dpad_x.state = 1;
                gamepad->dpad_y.state = 0;
                break;
            case HatSwitch_BottomRight:
                gamepad->dpad_x.state = 1;
                gamepad->dpad_y.state = 1;
                break;
            case HatSwitch_Bottom:
                gamepad->dpad_x.state = 0;
                gamepad->dpad_y.state = 1;
                break;
            case HatSwitch_BottomLeft:
                gamepad->dpad_x.state = -1;
                gamepad->dpad_y.state = 1;
                break;
            case HatSwitch_Left:
                gamepad->dpad_x.state = -1;
                gamepad->dpad_y.state = 0;
                break;
            case HatSwitch_TopLeft:
                gamepad->dpad_x.state = -1;
                gamepad->dpad_y.state = -1;
                break;
            default:
                gamepad->dpad_x.state = 0;
                gamepad->dpad_y.state = 0;
                break;
            }
        }
    }
}

static CFArrayRef get_array_property(IOHIDDeviceRef device, CFStringRef key)
{
    CFTypeRef ref = IOHIDDeviceGetProperty(device, key);
    if (ref != NULL && CFGetTypeID(ref) == CFArrayGetTypeID())
    {
        return (CFArrayRef)ref;
    }
    else
    {
        return NULL;
    }
}

internal void macos_device_callback(void *context, IOReturn result,
                                    void *sender, IOHIDDeviceRef device)
{
    if (result != kIOReturnSuccess)
    {
        return;
    }

    s32 vendorID  = 0;
    s32 productID = 0;

    {
        CFTypeRef ref =
            IOHIDDeviceGetProperty(device, CFSTR(kIOHIDVendorIDKey));
        if (ref)
        {
            if (CFGetTypeID(ref) == CFNumberGetTypeID())
            {
                CFNumberGetValue((CFNumberRef)ref, kCFNumberSInt32Type,
                                 &vendorID);
            }
        }
    }

    {
        CFTypeRef ref =
            IOHIDDeviceGetProperty(device, CFSTR(kIOHIDProductIDKey));
        if (ref)
        {
            if (CFGetTypeID(ref) == CFNumberGetTypeID())
            {
                CFNumberGetValue((CFNumberRef)ref, kCFNumberSInt32Type,
                                 &productID);
            }
        }
    }

    struct Gamepad *gamepad = (struct Gamepad *)context;

    if (vendorID == 0x054C && productID == 0x09CC)
    {
        printf("Sony Dualshock 4 detected\n");
        gamepad->face_left.usage_id   = 0x01;
        gamepad->face_bottom.usage_id = 0x02;
        gamepad->face_right.usage_id  = 0x03;
        gamepad->face_top.usage_id    = 0x04;
        gamepad->dpad_x.usage_id      = 0x39;
        gamepad->dpad_y.usage_id      = 0x39;
    }
    else if (vendorID == 0x57e && (productID == 0x2006 || productID == 0x2007))
    {
        printf("Nintendo Joy-Con (%c) detected\n",
               productID == 0x2006 ? 'L' : 'R');

        gamepad->face_bottom.usage_id = 0x01;
        gamepad->face_right.usage_id  = 0x02;
        gamepad->face_left.usage_id   = 0x03;
        gamepad->face_top.usage_id    = 0x04;
        gamepad->dpad_x.usage_id      = 0x39;
        gamepad->dpad_y.usage_id      = 0x39;
    }
    else
    {
        printf("vendorID: %x, productID: %x\n", vendorID, productID);
    }

    CFArrayRef matches = (__bridge CFArrayRef) @[
        @{@(kIOHIDElementUsagePageKey) : @(kHIDPage_Button)},
        @{@(kIOHIDElementUsagePageKey) : @(kHIDPage_GenericDesktop)}
    ];

    IOHIDDeviceSetInputValueMatchingMultiple(device, matches);

    IOHIDDeviceRegisterInputValueCallback(device, macos_device_input_callback,
                                          context);
}

internal void macos_init_gamepad(Gamepad *gamepad)
{
    IOHIDManagerRef manager =
        IOHIDManagerCreate(kCFAllocatorDefault, kIOHIDOptionsTypeNone);

    CFDictionaryRef gamepadRef = macos_device_matching_dictionary(
        kHIDPage_GenericDesktop, kHIDUsage_GD_GamePad);
    CFDictionaryRef multiAxisControllerRef = macos_device_matching_dictionary(
        kHIDPage_GenericDesktop, kHIDUsage_GD_MultiAxisController);
    CFDictionaryRef matchesList[] = {gamepadRef, multiAxisControllerRef};

    CFArrayRef matches =
        CFArrayCreate(kCFAllocatorDefault, (const void **)matchesList, 2, NULL);
    IOHIDManagerSetDeviceMatchingMultiple(manager, matches);

    IOHIDManagerRegisterDeviceMatchingCallback(manager, macos_device_callback,
                                               gamepad);

    IOHIDManagerScheduleWithRunLoop(manager, CFRunLoopGetMain(),
                                    kCFRunLoopDefaultMode);

    IOReturn ioReturn = IOHIDManagerOpen(manager, kIOHIDOptionsTypeNone);
    if (ioReturn != kIOReturnSuccess)
    {
        printf("an error occurred opening IOHIDManagerOpen\n");
        return;
    }
}
