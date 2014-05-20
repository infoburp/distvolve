#ifndef POLYGON_HPP_INCLUDED
#define POLYGON_HPP_INCLUDED
#include <vector>
#include <functional>

typedef unsigned short dimension;
typedef unsigned char byte;

namespace distvolve {
class Image {
public:
  void set(std::vector<byte> data, dimension width, dimension height);

  std::vector<byte> data;
  dimension width;
  dimension height;
};

class Polygon {
public:
  Polygon(byte serialized[],int location); //TODO create second constructor
  Polygon(dimension width,dimension height,float load,std::function <float()> rand);//random

  void drawInternal();

  void drawOn(Image &on,Image &from);
  bool drawOnIfBetter(Image &on,Image &from);

  unsigned int fitness(byte* one,byte* two);
  dimension random(dimension min,dimension max);
  std::vector<byte> serialize();

  std::vector<dimension> points;
  std::vector<bool> internalMask; // actually bitset-like
  byte colour[4];

  dimension width;
  dimension height;

  dimension centerx;
  dimension centery;
};
}
#endif // POLYGON_HPP_INCLUDED
