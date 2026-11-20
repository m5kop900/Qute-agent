#pragma once
// Smart chatbox positioning: pure geometry, no Qt types, no OS code (so it is unit-testable anywhere).
//
// The overlay is ONE OS window that holds the sprite, the chatbox ("card") and some invisible room
// below the card for dropdowns. The sprite's position on screen is an INPUT of this algorithm and is
// never changed by it: the card adapts to the sprite, the window is just the bounding box of both.
//
// All rectangles are in ONE coordinate space (global logical pixels of the monitor's work area).
// src/platform/placementscript.h holds a line-by-line JavaScript port for the compositor scripts
// (KWin, GNOME Shell) that must decide inside the compositor; tests/placement/ checks both agree.
#include <string>

namespace ChatboxPlacement {

struct Rect {
    int x = 0, y = 0, w = 0, h = 0;
    int right() const { return x + w; }
    int bottom() const { return y + h; }
    bool operator==(const Rect &o) const { return x == o.x && y == o.y && w == o.w && h == o.h; }
};

enum class Dir { Right, Left, Above, Below };
// Where the card sits along the axis that is NOT the direction.
//   Right/Left: End = card bottom on the sprite's bottom (card grows upward, the classic look),
//               Start = card top on the sprite's top (grows downward), Clamp = centred on the sprite, kept on screen.
//   Above/Below: End = card right edge on the sprite's right edge, Start = card left edge on the sprite's left edge.
enum class Align { End, Start, Clamp };

struct Input {
    Rect sprite;       // where the sprite IS (or will be) on screen: never modified
    int cardW = 0;     // actual runtime size of the visible card
    int cardH = 0;
    // cardW or cardH <= 0 means "minimized": no card, the window is the sprite alone.
    int dropRoom = 0;  // invisible room kept below the card (dropdowns); part of the window, not of validity
    int gap = 4;       // space between sprite and card
    Rect area;         // work area of the monitor that holds the sprite
};

struct Result {
    bool valid = false;   // card fully inside the area and not overlapping the sprite
    Dir dir = Dir::Right;
    Align align = Align::End;
    Rect card;            // visible card, global
    Rect window;          // bounding box of sprite + card + drop room, global
    // Offsets of the sprite and card inside `window`:
    int spriteX = 0, spriteY = 0, cardX = 0, cardY = 0;
    bool cardAnchoredBottom = true;   // true: the card's bottom edge is fixed while its height changes
};

Result choose(const Input &in);

// Wire format shared with the compositor scripts: "dir,align,wx,wy,ww,wh,sx,sy,cx,cy,cw,ch,anchorBottom,valid".
std::string toWire(const Result &r);
const char *dirName(Dir d);
const char *alignName(Align a);

} // namespace ChatboxPlacement
