#include <Polygon.hpp>
#include <mutex>
#include <atomic>
#include <iostream>
#include <vector>
#include <map>
#include <random>

using namespace std;
using namespace distvolve;

extern Image inputImage;
extern Image outputImage;
extern vector<byte> polys;
extern mutex mPoly;
extern atomic<int> generations;
extern atomic<int> polygons;
extern atomic<bool> run;

void doWork(char* identifier)
{
  float currentPolySize=.8;
  map<int,float> polySize;
  polySize[       0]=.80;
  polySize[    1000]=.70;
  polySize[   10000]=.50;
  polySize[   50000]=.40;
  polySize[  100000]=.35;
  polySize[  500000]=.28;
  polySize[ 1000000]=.2;
  polySize[ 4000000]=.1;

  srand(time(NULL));
  default_random_engine generator;
  normal_distribution<float> distribution(0.0,0.7);
  while(run)
  {
    if(polySize.begin()->first >= generations)
    {
      currentPolySize = polySize.begin()->second;
      polySize.erase(polySize.begin());
    }
    distvolve::Polygon poly(inputImage.width,inputImage.height,currentPolySize,[&distribution,&generator]()->float {return distribution(generator);});
    poly.drawInternal();
    if(poly.drawOnIfBetter(outputImage,inputImage))
    {
      mPoly.lock();
      vector<byte> vNewPoly = poly.serialize();
      polys.insert(polys.end(),vNewPoly.begin(),vNewPoly.end());
      mPoly.unlock();
      polygons++;
    }
    generations++;
  }
}
