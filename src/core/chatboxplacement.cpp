#include "chatboxplacement.h"

#include <algorithm>
#include <cmath>
#include <vector>

namespace ChatboxPlacement {

namespace {

struct Cand {
    Dir dir;
    Align align;
    Rect card;
};

int clampInt(int v, int lo, int hi) { return std::max(lo, std::min(v, std::max(lo, hi))); }

bool inside(const Rect &r, const Rect &a)
{
    return r.x >= a.x && r.y >= a.y && r.right() <= a.right() && r.bottom() <= a.bottom();
}

long long overlapArea(const Rect &a, const Rect &b)
{
    const int w = std::min(a.right(), b.right()) - std::max(a.x, b.x);
    const int h = std::min(a.bottom(), b.bottom()) - std::max(a.y, b.y);
    return (w > 0 && h > 0) ? static_cast<long long>(w) * h : 0;
}

Rect makeCard(const Input &in, Dir d, Align al)
{
    const Rect &s = in.sprite;
    Rect c;
    c.w = in.cardW;
    c.h = in.cardH;
    if (d == Dir::Right || d == Dir::Left) {
        c.x = d == Dir::Right ? s.right() + in.gap : s.x - in.gap - c.w;
        if (al == Align::End)        c.y = s.bottom() - c.h;
        else if (al == Align::Start) c.y = s.y;
        else                         c.y = clampInt(s.y + s.h / 2 - c.h / 2, in.area.y, in.area.bottom() - c.h);
    } else {
        c.y = d == Dir::Below ? s.bottom() + in.gap : s.y - in.gap - c.h;
        if (al == Align::End)        c.x = s.right() - c.w;
        else if (al == Align::Start) c.x = s.x;
        else                         c.x = clampInt(s.x + s.w / 2 - c.w / 2, in.area.x, in.area.right() - c.w);
    }
    return c;
}

bool valid(const Rect &card, const Input &in)
{
    return inside(card, in.area) && overlapArea(card, in.sprite) == 0;
}

// Higher is better: breathing room on screen, minus distance from the sprite. Room is capped so a huge
// monitor does not decide everything.
double score(const Rect &card, const Input &in)
{
    const int room = std::min({card.x - in.area.x, card.y - in.area.y,
                               in.area.right() - card.right(), in.area.bottom() - card.bottom()});
    const double cap = 200.0;
    const double clearance = std::min<double>(std::max(room, 0), cap);
    const double dx = (card.x + card.w / 2.0) - (in.sprite.x + in.sprite.w / 2.0);
    const double dy = (card.y + card.h / 2.0) - (in.sprite.y + in.sprite.h / 2.0);
    return clearance - 0.25 * std::sqrt(dx * dx + dy * dy);
}

Result finish(const Input &in, const Cand &c, bool isValid)
{
    Result r;
    r.valid = isValid;
    r.dir = c.dir;
    r.align = c.align;
    r.card = c.card;
    const Rect &s = in.sprite;
    const int left = std::min({s.x, c.card.x});
    const int top = std::min({s.y, c.card.y});
    const int right = std::max({s.right(), c.card.right()});
    const int bottom = std::max({s.bottom(), c.card.bottom() + in.dropRoom});
    r.window = Rect{left, top, right - left, bottom - top};
    r.spriteX = s.x - left;
    r.spriteY = s.y - top;
    r.cardX = c.card.x - left;
    r.cardY = c.card.y - top;
    // Cards that extend upward from the sprite's bottom (or sit above it) keep their bottom edge when the
    // height changes; the others keep the top edge.
    r.cardAnchoredBottom = (c.dir == Dir::Above) || ((c.dir == Dir::Right || c.dir == Dir::Left) && c.align != Align::Start);
    return r;
}

} // namespace

Result choose(const Input &in)
{
    // Minimized: no card at all. The window is the sprite alone.
    if (in.cardW <= 0 || in.cardH <= 0) {
        Result r;
        r.valid = true;
        r.card = Rect{in.sprite.x, in.sprite.y, 0, 0};
        r.window = in.sprite;
        return r;
    }

    // 1. The preferred position: right of the sprite, card growing upward ("top-right").
    const Cand preferred{Dir::Right, Align::End, makeCard(in, Dir::Right, Align::End)};
    if (valid(preferred.card, in))
        return finish(in, preferred, true);

    // 2. Everything else, in a fixed order (it only breaks exact ties).
    static const Dir dirs[] = {Dir::Above, Dir::Left, Dir::Below, Dir::Right};
    static const Align aligns[] = {Align::End, Align::Start, Align::Clamp};
    std::vector<Cand> all;
    for (Dir d : dirs)
        for (Align a : aligns) {
            if (d == Dir::Right && a == Align::End)
                continue;                                  // already tried
            all.push_back({d, a, makeCard(in, d, a)});
        }

    const Cand *best = nullptr;
    double bestScore = 0;
    for (const Cand &c : all) {
        if (!valid(c.card, in))
            continue;
        const double sc = score(c.card, in);
        if (!best || sc > bestScore + 1e-9) {
            best = &c;
            bestScore = sc;
        }
    }
    if (best)
        return finish(in, *best, true);

    // 3. Nothing fits (small screen, huge card): keep the card on screen and overlap the sprite as little
    //    as possible. The sprite still does not move.
    const Cand *least = nullptr;
    long long leastOverlap = 0;
    std::vector<Cand> forced;
    for (Dir d : dirs) {
        Cand c{d, Align::Clamp, makeCard(in, d, Align::Clamp)};
        c.card.x = clampInt(c.card.x, in.area.x, in.area.right() - c.card.w);
        c.card.y = clampInt(c.card.y, in.area.y, in.area.bottom() - c.card.h);
        forced.push_back(c);
    }
    for (const Cand &c : forced) {
        const long long ov = overlapArea(c.card, in.sprite);
        if (!least || ov < leastOverlap) {
            least = &c;
            leastOverlap = ov;
        }
    }
    return finish(in, *least, false);
}

const char *dirName(Dir d)
{
    switch (d) {
    case Dir::Right: return "right";
    case Dir::Left: return "left";
    case Dir::Above: return "above";
    case Dir::Below: return "below";
    }
    return "right";
}

const char *alignName(Align a)
{
    switch (a) {
    case Align::End: return "end";
    case Align::Start: return "start";
    case Align::Clamp: return "clamp";
    }
    return "end";
}

std::string toWire(const Result &r)
{
    return std::string(dirName(r.dir)) + "," + alignName(r.align) + "," + std::to_string(r.window.x) + ","
        + std::to_string(r.window.y) + "," + std::to_string(r.window.w) + "," + std::to_string(r.window.h) + ","
        + std::to_string(r.spriteX) + "," + std::to_string(r.spriteY) + "," + std::to_string(r.cardX) + ","
        + std::to_string(r.cardY) + "," + std::to_string(r.card.w) + "," + std::to_string(r.card.h) + ","
        + (r.cardAnchoredBottom ? "1" : "0") + "," + (r.valid ? "1" : "0");
}

} // namespace ChatboxPlacement
