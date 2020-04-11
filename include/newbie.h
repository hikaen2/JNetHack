/*
  $Id:$
 */

#ifndef NEWBIE_H
#define NEWBIE_H

#include "global.h"

struct newbie{
     Bitfield(search, 1);
     Bitfield(pickup, 1);
     Bitfield(unlock, 1);
     Bitfield(open, 1);
     Bitfield(down, 1);
     Bitfield(pray, 1);
     Bitfield(fainted, 1);
     Bitfield(weak, 1);
     Bitfield(eat, 1);
     Bitfield(loot, 1);
     Bitfield(offer, 1);

     int try_open;
     int found_altar;
     int found_chest;
} newbie;
#endif
