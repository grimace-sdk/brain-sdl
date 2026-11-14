//------------------------------------------------------------------------------
//
//  File:  brain_present.h
//
//  Display escapes for full-screen apps that draw into their own RGB565
//  buffers and present them straight to the panel on its tearing effect (TE)
//  pulse. GDI keeps its normal mode; its updates are held while an app
//  presents and redrawn when it closes or exits. The PXP cannot scale RGB,
//  so Scale must be 1.
//
//  Up to eight overlays, each with one or two buffers of its own, are
//  blended over each frame by the PXP as it copies the frame, so an app can
//  keep a HUD out of its frames. Overlays are placed in the app's image in
//  8x8 blocks and are not scaled. Lower numbers are in front, and where
//  overlays overlap only the front one counts: its transparent pixels show
//  the frame, not the overlays behind it. RGB565
//  overlays are opaque, or transparent where they match ColourKey (RGB565,
//  shared by all overlays) with BRAIN_PRESENT_OVERLAY_COLOUR_KEY; ARGB1555
//  overlays are transparent where the top bit is clear. All overlays start
//  hidden. Overlay buffers hold 16 bits per pixel, Width pixels per row.
//  PRESENT with a BrainPresentFrame shows an overlay buffer, keeps
//  the current one or hides the overlay; the buffer shown is read for every
//  frame, so draw into the other one and show that. WAIT_OVERLAY returns
//  once a buffer is no longer shown or being read.
//
//  Use with ExtEscape on a screen DC:
//
//      OPEN            in BrainPresentOpen, out BrainPresentInfo
//      PRESENT         in DWORD buffer index or BrainPresentFrame,
//                      out DWORD replaced index (optional)
//      WAIT            in DWORD buffer index
//      CLOSE           no input
//      STATS           in DWORD reset (optional), out BrainPresentStats
//      WAIT_OVERLAY    in BrainPresentOverlayBuffer
//
//------------------------------------------------------------------------------
#ifndef __BRAIN_PRESENT_H__
#define __BRAIN_PRESENT_H__

#define BRAIN_PRESENT_ESCAPE_BASE   100100
#define BRAIN_PRESENT_OPEN          (BRAIN_PRESENT_ESCAPE_BASE + 58)
#define BRAIN_PRESENT               (BRAIN_PRESENT_ESCAPE_BASE + 59)
#define BRAIN_PRESENT_WAIT          (BRAIN_PRESENT_ESCAPE_BASE + 60)
#define BRAIN_PRESENT_CLOSE         (BRAIN_PRESENT_ESCAPE_BASE + 61)
#define BRAIN_PRESENT_STATS         (BRAIN_PRESENT_ESCAPE_BASE + 62)
#define BRAIN_PRESENT_WAIT_OVERLAY  (BRAIN_PRESENT_ESCAPE_BASE + 63)

#define BRAIN_PRESENT_MAX_BUFFERS   2
#define BRAIN_PRESENT_MAX_OVERLAYS  8
#define BRAIN_PRESENT_NONE          0xFFFFFFFF
#define BRAIN_PRESENT_KEEP          0xFFFFFFFF
#define BRAIN_PRESENT_HIDE          0xFFFFFFFE

#define BRAIN_PRESENT_OVERLAY_RGB565        1
#define BRAIN_PRESENT_OVERLAY_ARGB1555      2
#define BRAIN_PRESENT_OVERLAY_COLOUR_KEY    0x00000001

#define BRAIN_PRESENT_FAST_CPU      0x00000001

#define BRAIN_PRESENT_PRESENTING    0x00000001
#define BRAIN_PRESENT_SUSPENDED     0x00000002
#define BRAIN_PRESENT_TE            0x00000004

typedef struct
{
    DWORD Left;
    DWORD Top;
    DWORD Width;
    DWORD Height;
    DWORD Format;
    DWORD Buffers;
    DWORD Flags;
} BrainPresentOverlay;

typedef struct
{
    DWORD Width;
    DWORD Height;
    DWORD Scale;
    DWORD Buffers;
    DWORD Background;
    DWORD Flags;
    DWORD ColourKey;
    DWORD Overlays;
    BrainPresentOverlay Overlay[BRAIN_PRESENT_MAX_OVERLAYS];
} BrainPresentOpen;

typedef struct
{
    UINT16* pBuffers[BRAIN_PRESENT_MAX_BUFFERS];
    DWORD Stride;
    DWORD Width;
    DWORD Height;
    DWORD Scale;
    DWORD Flags;
    UINT16* pOverlayBuffers[BRAIN_PRESENT_MAX_OVERLAYS][BRAIN_PRESENT_MAX_BUFFERS];
} BrainPresentInfo;

typedef struct
{
    DWORD Buffer;
    DWORD Overlay[BRAIN_PRESENT_MAX_OVERLAYS];
} BrainPresentFrame;

typedef struct
{
    DWORD Overlay;
    DWORD Buffer;
} BrainPresentOverlayBuffer;

typedef struct
{
    DWORD Frames;
    DWORD Replaced;
    DWORD Dropped;
    DWORD CleanMicroseconds;
    DWORD PxpMicroseconds;
    DWORD LatencyMicroseconds;
    DWORD Submitted;
    DWORD TeTimeouts;
    DWORD State;
    DWORD Orientation;
} BrainPresentStats;

#endif /* __BRAIN_PRESENT_H__ */
