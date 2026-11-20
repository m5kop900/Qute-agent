// Unit test of the smart chatbox positioning geometry (src/core/chatboxplacement). Plain C++ on purpose: the
// algorithm has no Qt types, so this test builds and runs without Qt. Exit code 0 = pass.
#include "chatboxplacement.h"

#include <cstdio>
#include <cstdlib>
#include <string>

using namespace ChatboxPlacement;

static int g_failures = 0;
#define CHECK(cond, msg)                                                                           \
    do {                                                                                           \
        if (!(cond)) {                                                                             \
            ++g_failures;                                                                          \
            std::fprintf(stderr, "FAIL %s:%d  %s  (%s)\n", __FILE__, __LINE__, #cond, msg);        \
        }                                                                                          \
    } while (0)

static const Rect kScreen{0, 0, 1920, 1080};
static const int kSprite = 128, kCardW = 422, kDrop = 200, kGap = 4;

static Input make(int sx, int sy, int cardH, Rect area = kScreen)
{
    Input in;
    in.sprite = Rect{sx, sy, kSprite, kSprite};
    in.cardW = kCardW;
    in.cardH = cardH;
    in.dropRoom = kDrop;
    in.gap = kGap;
    in.area = area;
    return in;
}

static bool insideArea(const Rect &r, const Rect &a)
{
    return r.x >= a.x && r.y >= a.y && r.right() <= a.right() && r.bottom() <= a.bottom();
}

static bool overlaps(const Rect &a, const Rect &b)
{
    return std::min(a.right(), b.right()) > std::max(a.x, b.x) && std::min(a.bottom(), b.bottom()) > std::max(a.y, b.y);
}

// The sprite's global rectangle follows from the window and its offset: it must be the input, always.
static void checkInvariants(const Input &in, const Result &r, const char *name)
{
    CHECK(r.window.x + r.spriteX == in.sprite.x && r.window.y + r.spriteY == in.sprite.y, name);
    CHECK(r.window.x + r.cardX == r.card.x && r.window.y + r.cardY == r.card.y, name);
    CHECK(r.card.w == in.cardW && r.card.h == in.cardH, name);
    CHECK(r.cardX + r.card.w <= r.window.w && r.cardY + r.card.h + in.dropRoom <= r.window.h, name);
    CHECK(r.spriteX >= 0 && r.spriteY >= 0 && r.spriteX + in.sprite.w <= r.window.w && r.spriteY + in.sprite.h <= r.window.h, name);
    if (r.valid) {
        CHECK(insideArea(r.card, in.area), name);
        CHECK(!overlaps(r.card, in.sprite), name);
    }
}

static void topRightIsPreferred()
{
    // Middle of the screen, bottom-left, left edge: the card fits to the right and grows upward.
    const int pos[][2] = {{896, 476}, {0, 1080 - kSprite}, {0, 476}, {896, 1080 - kSprite}};
    for (const auto &p : pos) {
        const Input in = make(p[0], p[1], 250);
        const Result r = choose(in);
        CHECK(r.valid && r.dir == Dir::Right && r.align == Align::End, "top-right preferred when valid");
        // The classic arrangement: card right of the sprite, bottom edges level.
        CHECK(r.card.x == in.sprite.right() + kGap && r.card.bottom() == in.sprite.bottom(), "classic card rectangle");
        checkInvariants(in, r, "top-right");
    }
}

static void cornersAndEdges()
{
    struct Case { const char *name; int x, y; bool dirOk[4]; };   // allowed: right, left, above, below
    const int R = 1920 - kSprite, B = 1080 - kSprite;
    const Case cases[] = {
        {"bottom-right: above or left", R, B, {false, true, true, false}},
        {"top-right: below or left", R, 0, {false, true, false, true}},
        {"bottom-left: above or right", 0, B, {true, false, true, false}},
        {"top-left: below or right", 0, 0, {true, false, false, true}},
        {"right edge: left or above/below", R, 476, {false, true, true, true}},
        {"top edge: below or right/left", 896, 0, {true, true, false, true}},
    };
    for (const Case &c : cases) {
        for (int h : {140, 250, 430}) {
            const Input in = make(c.x, c.y, h);
            const Result r = choose(in);
            const bool ok = c.dirOk[static_cast<int>(r.dir)];
            CHECK(ok, c.name);
            CHECK(r.valid, c.name);
            checkInvariants(in, r, c.name);
        }
    }
    // The case the spec calls out: bottom-right never gets the card pushed off screen to the right.
    const Result r = choose(make(1920 - kSprite, 1080 - kSprite, 430));
    CHECK(r.card.right() <= 1920 && r.card.bottom() <= 1080, "bottom-right stays on screen");
}

static void spriteNeverMoves()
{
    // Many sprite positions, three card heights, two areas: the sprite offset always maps back to the input.
    int n = 0;
    for (const Rect area : {kScreen, Rect{-1280, 100, 1280, 720}})
        for (int x = area.x; x <= area.right() - kSprite; x += 97)
            for (int y = area.y; y <= area.bottom() - kSprite; y += 89)
                for (int h : {140, 250, 430}) {
                    const Input in = make(x, y, h, area);
                    checkInvariants(in, choose(in), "grid");
                    ++n;
                }
    CHECK(n > 500, "grid size");
}

static void multiMonitorUsesTheSpritesArea()
{
    // Second monitor to the left of the primary (negative x): placement stays inside THAT monitor's work area.
    const Rect left{-1280, 100, 1280, 720};
    const Input in = make(-1280 + 20, 100 + 720 - kSprite, 430, left);   // bottom-left of the left monitor
    const Result r = choose(in);
    CHECK(r.valid && insideArea(r.card, left), "inside the left monitor");
    const Input edge = make(-kSprite, 300, 250, left);                    // against its right edge
    const Result re = choose(edge);
    CHECK(re.valid && insideArea(re.card, left) && re.dir != Dir::Right, "does not spill onto the next monitor");
}

static void chatboxChangesButSpriteDoesNot()
{
    // A card that grows while the sprite sits near the top: the direction may change, the sprite never does.
    const int sx = 600, sy = 300;
    for (int h : {140, 250, 430, 1000}) {
        const Input in = make(sx, sy, h);
        const Result r = choose(in);
        checkInvariants(in, r, "growing card");
        CHECK(r.window.x + r.spriteX == sx && r.window.y + r.spriteY == sy, "sprite fixed while the card grows");
    }
}

static void nothingFits()
{
    // Tiny work area: no valid placement. The card stays inside the area where it can, the sprite does not move.
    const Rect tiny{0, 0, 500, 400};
    const Input in = make(100, 100, 430, tiny);   // the card is taller than the area
    const Result r = choose(in);
    CHECK(!r.valid, "no valid placement");
    CHECK(r.window.x + r.spriteX == 100 && r.window.y + r.spriteY == 100, "sprite not moved in the fallback");
    CHECK(r.card.x >= tiny.x && r.card.right() <= tiny.right(), "fallback keeps the card within the width");
}

static void minimizedIsJustTheSprite()
{
    Input in = make(300, 500, 0);
    in.cardW = 0;
    const Result r = choose(in);
    CHECK(r.valid && r.window == in.sprite && r.spriteX == 0 && r.spriteY == 0, "window is the sprite");
}

static void wireFormat()
{
    const Input in = make(0, 1080 - kSprite, 250);
    const std::string w = toWire(choose(in));
    int commas = 0;
    for (char c : w)
        commas += c == ',';
    CHECK(commas == 13, "14 wire fields");
    CHECK(w.rfind("right,end,", 0) == 0, "starts with direction and alignment");
}

int main()
{
    topRightIsPreferred();
    cornersAndEdges();
    spriteNeverMoves();
    multiMonitorUsesTheSpritesArea();
    chatboxChangesButSpriteDoesNot();
    nothingFits();
    minimizedIsJustTheSprite();
    wireFormat();
    if (g_failures == 0)
        std::puts("tst_chatboxplacement: all checks passed");
    return g_failures == 0 ? 0 : 1;
}
