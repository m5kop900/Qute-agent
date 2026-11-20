// Std-only driver: reads inputs (one per line: sx sy sw sh cardW cardH drop gap ax ay aw ah) from stdin and
// prints ChatboxPlacement::toWire() for each. Used by parity.sh to compare with the JavaScript port.
#include "chatboxplacement.h"
#include <iostream>
int main()
{
    ChatboxPlacement::Input in;
    while (std::cin >> in.sprite.x >> in.sprite.y >> in.sprite.w >> in.sprite.h >> in.cardW >> in.cardH
           >> in.dropRoom >> in.gap >> in.area.x >> in.area.y >> in.area.w >> in.area.h)
        std::cout << ChatboxPlacement::toWire(ChatboxPlacement::choose(in)) << "\n";
}
