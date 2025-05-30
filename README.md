# Handmade Hero
Handmade Hero is a series by Casey Moratori to develop a video game from scratch. This series does not use a additional libraries (besides OS specific ones). Here we do everything from scratch as much as possible. 

From the series spawned the recent Handmade movement we us developers favor reinventing the wheel. We do this to learn, to maintain control of our code and projects, to get away from unnecessary abstractions, to get away from dependencies.

### About the series
- The series primarily uses Windows, but the game will support for other platforms. I am starting off following along on a Mac.
- Casey uses C-styles C++ for the project. Because of this I am following along simply with C (Objective-C for the Mac platform).
- No fancy build tools are use. Just simple bash/bat scripts.

### Resources
- Official videos: https://guide.handmadehero.org/code/
- Community notes: https://yakvi.github.io/handmade-hero-notes/index.html
- Casey's newer series: https://www.computerenhance.com/
- Unofficial Mac Platform Layer: https://www.youtube.com/playlist?list=PLQOu9z2IsoWnvByDqg_CmihZogT5sI3uM
- Unofficial Linux Platform Layer: https://davidgow.net/handmadepenguin/default.html

### Day 010 - Calculate Frames per Second
```
struct mach_timebase_info timebase_info;
mach_timebase_info(&timebase_info);
u64 last_clock_tick = mach_absolute_time();

while (RUNNING)
{
    ...

    u64 end_clock_tick = mach_absolute_time();
    last_clock_tick    = end_clock_tick;

    u64 elapsed_clock_tick = end_clock_tick - last_clock_tick;
    u64 elapsed_time_ns =
        elapsed_clock_tick * timebase_info.numer / timebase_info.denom;
    f32 elapsed_time_s = (f32)elapsed_time_ns * 1.0E-9; // seconds / frame
    f32 fps            = 1.f / elapsed_time_s;

    NSLog(@"frames/second %.02ffps", fps);
}
```

### Day 009 - Variable-Pitch Sine Wave Output
### Day 008 - Writing to the Sound Buffer
### Day 007 - Allocation a Sound Buffer
### Day 006 - Gamepad and Keyboard Input
### Day 005 - Review
### Day 004 - Animating the Back Buffer
### Day 003 - Allocation a Back Buffer
### Day 002 - Opening a Window
### Day 001 - Setting up the Build