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

  int numFails=0;
  srand(time(NULL));
  default_random_engine generator;
  normal_distribution<float> distribution(0.0,0.7);
  while(run)
  {
    distvolve::Polygon poly(inputImage.width,inputImage.height,currentPolySize,[&distribution,&generator]()->float {return distribution(generator);});
    poly.drawInternal();
    if(poly.drawOnIfBetter(outputImage,inputImage))
    {
      mPoly.lock();
      vector<byte> vNewPoly = poly.serialize();
      polys.insert(polys.end(),vNewPoly.begin(),vNewPoly.end());
      mPoly.unlock();
      polygons++;
      numFails=0;
    }
    else
    {
      if(numFails++>400)
      {
        currentPolySize*=.08;
        cout << "Dropped to " << currentPolySize << "\n";
        numFails=0;
      }
    }
    generations++;
  }
}
