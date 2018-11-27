#include "stdbool.h"
#include "Enums.h"

#ifndef BOMB_H
#define BOMB_H

struct Projectile
{
  struct Position *Position;
  char Symbol;
  Direction Direction;
  bool Collision;
};

#endif
