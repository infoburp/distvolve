#include<Polygon.hpp>
#include<iostream>
#include<algorithm>
#include<cmath>
#include<thread>
#include<mutex>
#include<atomic>
#include<iterator>

using namespace std;
using namespace distvolve;

extern atomic<int> readingData;
extern mutex writingData;
void pushBackDimension(vector<byte> &ret,dimension *d);

void Image::set(std::vector<byte> data, dimension width, dimension height)
{
  this->data = data;
  this->width=width;
  this->height=height;
};

dimension Polygon::random(dimension min,dimension max)
{
  if(max==min) return max;
  return rand()%(max-min)+min+1;
};

vector<byte> Polygon::serialize()
{
  vector<byte> r;
  size_t ps = this->points.size();
  for(int i=4;i>0;i--)
    r.push_back(((byte*)&ps)[i]);
  r.push_back(colour[0]);
  r.push_back(colour[1]);
  r.push_back(colour[2]);
  r.push_back(colour[3]);
  for(int i = 0; i<this->points.size();i++)
  {
    pushBackDimension(r,this->points.data()+i);
  }
  return r;
}

dimension rd(float a)
{
  return lroundf(a);
}

Polygon::Polygon(dimension width,dimension height,float load,std::function <float()> rand)
{
  this->width = width;
  this->height = height;

  this->internalMask.resize(width*height,false);

  this->points.resize(6,0);

  vector<dimension>p(3);
  p[0]=this->random(0,width-1);
  p[1]=p[0]+rd(rand()*load*width);
  if(p[1]<0)p[1]=0;
  if(p[1]>=width)p[1]=width-1;
  p[2]=p[0]+rd(rand()*load*width);
  if(p[2]<0)p[2]=0;
  if(p[2]>=width)p[2]=width-1;

  byte n=0;
  if(p[0]>p[1])
    n=0;
  else
    n=1;
  if(p[2]>p[n])
    n=2;
  this->points[0]=p[n];
  p.erase(p.begin()+n);
  this->points[2]=p[0];
  this->points[4]=p[1];

  vector<dimension>o(3);
  o[0]=this->random(0,height-1);
  o[1]=o[0]+rd(rand()*load*height);
  if(o[1]<0)o[1]=0;
  if(o[1]>=height)o[1]=height-1;
  o[2]=o[0]+rd(rand()*load*height);
  if(o[2]<0)o[2]=0;
  if(o[2]>=height)o[2]=height-1;

  if(o[0]<o[1])
    n=0;
  else
    n=1;
  if(o[2]<o[n])
    n=2;
  this->points[1]=o[n];
  o.erase(o.begin()+n);
  this->points[3]=o[0];
  this->points[5]=o[1];

  this->colour[3] = this->random(0,200);
  this->centerx = (this->points[0]+this->points[2]+this->points[4])/3;
  this->centery = (this->points[1]+this->points[3]+this->points[5])/3;
};
byte r(float a)
{
  return lroundf(a);
}

unsigned int Polygon::fitness(byte *a,byte *b)
{
  unsigned short fitness=0;
  for(byte i=0;i<3;i++)
  {
    if(a[i]>b[i])
      fitness+=a[i]-b[i];
    else
      fitness+=b[i]-a[i];
  }
  return fitness;
};

bool Polygon::drawOnIfBetter(Image &outImage,Image &from)
{
  vector<byte> tempImageVector(outImage.width*outImage.height*4,255);
  Image image;
  image.set(tempImageVector,image.width,image.height);
  byte *fromColour = from.data.data()+(centerx+centery*from.width)*4;
  this->colour[0]= fromColour[0];
  this->colour[1]= fromColour[1];
  this->colour[2]= fromColour[2];
  this->colour[3]*= fromColour[3];

  int currentFitness=0;
  int newFitness=0;

  float sa = float(this->colour[3])/255.0;

  writingData.lock();
  readingData++;
  writingData.unlock();
  for(int x=0;x<image.data.size();x+=4)
    {
    byte* colour = image.data.data()+x;
    byte* outColour = outImage.data.data()+x;
    if(this->internalMask[x/4])
    {//inside the tri
      byte* original = from.data.data()+x;

      float da = float(outColour[3])/255.0;
      float a  = sa+da*(1-sa);
      if(a<=1.0/255.0)
      {
        colour[0]=0;
        colour[1]=0;
        colour[2]=0;
        colour[3]=0;
      }
      else
      {
        colour[0]=r(((outColour[0]*sa)+(this->colour[0]*da*(1-sa)))/a);
        colour[1]=r(((outColour[1]*sa)+(this->colour[1]*da*(1-sa)))/a);
        colour[2]=r(((outColour[2]*sa)+(this->colour[2]*da*(1-sa)))/a);
        colour[3]=r(a*255);
      }
      currentFitness+=Polygon::fitness(original,outColour);// lower is better because fast
      newFitness+=Polygon::fitness(original,colour);
    }
    else
    {
      colour[0]=outColour[0];
      colour[1]=outColour[1];
      colour[2]=outColour[2];
      colour[3]=outColour[3];
    }
  }
  readingData--;
  if(currentFitness>newFitness)
  { // fitness is higher by any amount.
    int area = abs(((this->points[0]-this->points[4])*(this->points[3]-this->points[1]))
                -((this->points[0]-this->points[2])*(this->points[5]-this->points[1])))/2;
    if (area*5>currentFitness-newFitness)
        return false;
    writingData.lock();
    while(readingData>0)
      std::this_thread::yield();
    outImage.data = image.data;
    writingData.unlock();
    return true;
  }
  return false;
};
void Polygon::drawOn(Image &image,Image &from)
{
  byte *fromColour = from.data.data()+(centerx+centery*from.width)*4;
  this->colour[0]= fromColour[0];
  this->colour[1]= fromColour[1];
  this->colour[2]= fromColour[2];
  this->colour[3]*= fromColour[3];

  float sa = float(this->colour[3])/255.0;

  for(size_t x=0;x<image.data.size();x+=4) {
    if(this->internalMask[x/4])
    {
      byte* colour = image.data.data()+x;
      float da = float(colour[3])/255.0;
      float a  = sa+da*(1-sa);
      if(a<=1.0/255.0)
      {
        colour[0]=0;
        colour[1]=0;
        colour[2]=0;
        colour[3]=0;
      }
      else
      {
        colour[0]=r(((colour[0]*sa)+(this->colour[0]*da*(1-sa)))/a);
        colour[1]=r(((colour[1]*sa)+(this->colour[1]*da*(1-sa)))/a);
        colour[2]=r(((colour[2]*sa)+(this->colour[2]*da*(1-sa)))/a);
        colour[3]=r(a*255) ;
      }
    }
  }
};
void Polygon::drawInternal()
{
  if(this->points.size()!=6)
    return;

  // see http://devmaster.net/posts/6145/advanced-rasterization

  const int X1 = this->points[0]*16;
  const int X2 = this->points[2]*16;
  const int X3 = this->points[4]*16;
  const int Y1 = this->points[1]*16;
  const int Y2 = this->points[3]*16;
  const int Y3 = this->points[5]*16;


  // Deltas
  const int DX12 = X1 - X2;
  const int DX23 = X2 - X3;
  const int DX31 = X3 - X1;

  const int DY12 = Y1 - Y2;
  const int DY23 = Y2 - Y3;
  const int DY31 = Y3 - Y1;

  // Fixed-point deltas
  const int FDX12 = DX12 << 4;
  const int FDX23 = DX23 << 4;
  const int FDX31 = DX31 << 4;

  const int FDY12 = DY12 << 4;
  const int FDY23 = DY23 << 4;
  const int FDY31 = DY31 << 4;

  // Bounding rectangle
  int minx = (min(min(X1, X2), X3) + 0xF) >> 4;
  int maxx = (max(max(X1, X2), X3) + 0xF) >> 4;
  int miny = (min(min(Y1, Y2), Y3) + 0xF) >> 4;
  int maxy = (max(max(Y1, Y2), Y3) + 0xF) >> 4;

  // Block size, standard 8x8 (must be power of two)
  const int q = 8;

  // Start in corner of 8x8 block
  minx &= ~(q - 1);
  miny &= ~(q - 1);

  // Half-edge constants
  int C1 = DY12 * X1 - DX12 * Y1;
  int C2 = DY23 * X2 - DX23 * Y2;
  int C3 = DY31 * X3 - DX31 * Y3;

  // Correct for fill convention
  if(DY12 < 0 || (DY12 == 0 && DX12 > 0)) C1++;
  if(DY23 < 0 || (DY23 == 0 && DX23 > 0)) C2++;
  if(DY31 < 0 || (DY31 == 0 && DX31 > 0)) C3++;

  int index=miny*this->width;

  // Loop through blocks
  for(int y = miny; y < maxy; y += q)
  {
    for(int x = minx; x < maxx; x += q)
    {
      // Corners of block
      int x0 = x << 4;
      int x1 = (x + q - 1) << 4;
      int y0 = y << 4;
      int y1 = (y + q - 1) << 4;

      // Evaluate half-space functions
      bool a00 = C1 + DX12 * y0 - DY12 * x0 > 0;
      bool a10 = C1 + DX12 * y0 - DY12 * x1 > 0;
      bool a01 = C1 + DX12 * y1 - DY12 * x0 > 0;
      bool a11 = C1 + DX12 * y1 - DY12 * x1 > 0;
      int a = (a00 << 0) | (a10 << 1) | (a01 << 2) | (a11 << 3);

      bool b00 = C2 + DX23 * y0 - DY23 * x0 > 0;
      bool b10 = C2 + DX23 * y0 - DY23 * x1 > 0;
      bool b01 = C2 + DX23 * y1 - DY23 * x0 > 0;
      bool b11 = C2 + DX23 * y1 - DY23 * x1 > 0;
      int b = (b00 << 0) | (b10 << 1) | (b01 << 2) | (b11 << 3);

      bool c00 = C3 + DX31 * y0 - DY31 * x0 > 0;
      bool c10 = C3 + DX31 * y0 - DY31 * x1 > 0;
      bool c01 = C3 + DX31 * y1 - DY31 * x0 > 0;
      bool c11 = C3 + DX31 * y1 - DY31 * x1 > 0;
      int c = (c00 << 0) | (c10 << 1) | (c01 << 2) | (c11 << 3);

      // Skip block when outside an edge
      if(a == 0x0 || b == 0x0 || c == 0x0) continue;

      int tIndex = index;

      // Accept whole block when totally covered
      if(a == 0xF && b == 0xF && c == 0xF)
      {
        for(int iy = 0; iy < q; iy++)
        {
          for(int ix = x; ix < x + q; ix++)
          {
            this->internalMask[tIndex+ix] = true;
          }
          tIndex += this->width;
        }
      }
      else // Partially covered block
      {
        int CY1 = C1 + DX12 * y0 - DY12 * x0;
        int CY2 = C2 + DX23 * y0 - DY23 * x0;
        int CY3 = C3 + DX31 * y0 - DY31 * x0;
        for(int iy = y; iy < y + q; iy++)
        {
          int CX1 = CY1;
          int CX2 = CY2;
          int CX3 = CY3;
          for(int ix = x; ix < x + q; ix++)
          {
            if(CX1 > 0 && CX2 > 0 && CX3 > 0)
            {
              this->internalMask[tIndex+ix] = true;
            }
            CX1 -= FDY12;
            CX2 -= FDY23;
            CX3 -= FDY31;
          }
          CY1 += FDX12;
          CY2 += FDX23;
          CY3 += FDX31;
          tIndex += this->width;
        }
      }
    }
    index+= q*this->width;
  }
};
