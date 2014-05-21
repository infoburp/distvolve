#include <iostream>
#include <stdlib.h>
#include <lodepng.h>
#include <Polygon.hpp>
#include <ThreadWorker.hpp>
#include <string>
#include <mutex>
#include <atomic>
#include <fstream>
#include <algorithm>
#include <thread>

using namespace std;
using namespace distvolve;

Image inputImage;
Image outputImage;
vector<byte> polys;
mutex mPoly;
atomic<int> readingData;
mutex writingData;
atomic<int> generations(0);
atomic<int> polygons(0);
atomic<bool> run(true);
bool loadedImage=false;
unsigned width,height;
vector<thread> threads ={};

void saveImage(string file)
{
  writingData.lock();
  readingData++;
  writingData.unlock();
  lodepng::encode(file,outputImage.data,(unsigned)width,(unsigned)height);
  readingData--;
}

void pushBackDimension(vector<byte> &ret,dimension *d)
{
  byte* v = (byte*)d;
  for(int i=sizeof(dimension)-1;i>=0;i--)
  {
    ret.push_back(v[i]);
  }
  return;
}

vector<byte> dna()
{
  vector<byte> ret;
  ret.push_back(0x60);
  size_t dimensionSizeS = sizeof(dimension);
  byte* dimensionSize = (byte*)&dimensionSizeS;
  short n=sizeof(size_t);
  ret.push_back(dimensionSize[1]); // short of size_t
  ret.push_back(dimensionSize[0]);
  pushBackDimension(ret,(dimension*)&width);
  pushBackDimension(ret,(dimension*)&height);
  mPoly.lock();
  size_t ps = polys.size();
  for(int i=sizeof(int)-1;i>=0;i--)
    ret.push_back(((byte*)&ps)[i]);
  ret.insert(ret.end(),polys.begin(),polys.end());
  mPoly.unlock();
  return ret;
}

void saveDna(string file)
{
  return;
  ofstream filestream(file,ofstream::trunc|ofstream::out|ofstream::binary);
  vector <byte> vDna = dna();
  filestream.write((char*)vDna.data(),vDna.size());
  filestream.close();
}

dimension readDimension(ifstream &filestream)
{
  dimension d = 0;
  for(size_t i=sizeof(dimension)-1;(int)i>=0;i--)
  {
    filestream.read((char*)&((byte*)&d)[i],1);
  }
  return d;
}

int loadDna(string file)
{
  return 1;
  ifstream filestream(file,ifstream::in|ifstream::binary);
  byte magic;
  filestream.read((char*)&magic,1);
  if((int)magic!=0x60)
    return 1;
  size_t dimensionSize=0;
  filestream.read((char*)&dimensionSize+1,1);
  filestream.read((char*)&dimensionSize,1);
  if(dimensionSize!=sizeof(dimension))
  {
    cout << "Unable to read dna. (Dimension size)\n";
    return 1;
  }
  if(readDimension(filestream)!=width || readDimension(filestream)!=height)
  {
    cout << "Non matching dimensions!\n" ;
    return 1;
  }
  while(!filestream.eof())
  {
    Polygon newPoly(width,height,filestream);
    newPoly.drawInternal();
    newPoly.drawOnFromDna(outputImage);
    vector <byte> vNewPoly = newPoly.serialize();
    polys.insert(polys.end(),vNewPoly.begin(),vNewPoly.end());
  }
  filestream.close();
  return 0;
}

unsigned loadImage(string file)
{
  vector<byte> importImage;
  unsigned lodeerr = lodepng::decode(importImage,width,height,file);
  if(lodeerr)
  {
    cout << "Error: " << lodepng_error_text(lodeerr) << "\n";
    return lodeerr;
  }
  vector<byte> imageVector(width*height*4,255);
  inputImage.set(importImage,width,height);
  outputImage.set(imageVector,width,height);
  return 0;
}

void stopThreads()
{
  run=false;
  for(auto& i:threads)
    i.join();
  threads.clear();
}

void startThreads(short numThreads)
{
  if(!loadedImage)
  {
    cout << "You must load an image first" << endl;
    return;
  }
  for(short thread=0;thread<numThreads;thread++)
  {
    char id[] = "        ";
    snprintf(id,8,"%i",thread);
    threads.push_back(std::thread(doWork,id));
  }
}

int main()
{
  string endl = "\n$ ";
  cout << "DISTVOLVE V0.1.0" << endl;
  while(true)
  {
    char cLine[256];
    cin.getline(cLine,256);
    string line(cLine);
    string command;
    string args;
    transform(line.begin(),line.end(),line.begin(),::tolower); // lower case
    size_t split = line.find(" ");
    if(split==string::npos)
      command = line;
    else
    {
      command = line.substr(0,split);
      args = line.substr(split+1);
    }
    if(command=="start" || command=="b")
    {
      short threadsToStart = atoi(args.c_str());
      if(threadsToStart<1)
      {
        cout << "You must specify 1+ threads to start" << endl;
      }
      else
      {
        startThreads(threadsToStart);
        cout << "Started " << threadsToStart << " threads." << endl;
      }
    } else
    if(command=="stop" || command=="c")
    {
      stopThreads();
      cout << "Stopped threads" << endl;
    } else
    if(command=="save" || command=="s")
    {
      if(args.empty())
      {
        cout << "Must specify a filename" << endl;
      }
      else
      {
        saveImage(args);
        cout << "Saved " << args << endl;
      }
    } else
    if(command=="load" || command=="l")
    {
      if(!threads.empty())
      {
        cout << "Must stop threads before loading new file" << endl;
      }
      else
      {
        if(args.empty())
        {
          cout << "Must specify a filename" << endl;
        }
        else
        {
          if(!loadImage(args))
          {
            loadedImage=true;
            cout << "Loaded " << args << endl;
          }
          else
          {
            cout << "Error loading image" << endl;
          }
        }
      }
    } else
    if(command=="progress" || command=="p")
    {
      cout << polygons << " polygons / " << generations << " generations." << endl;
    } else
    if(command=="savedna" || command=="d")
    {
      if(args.empty())
      {
        cout << "Must specify a filename" << endl;
      }
      else
      {
        saveDna(args);
        cout << "Saved " << args << endl;
      }
    } else
    if(command=="loaddna" || command=="r")
    {
      if(!threads.empty())
      {
        cout << "Must stop threads before loading new file" << endl;
      }
      else
      {
        if(args.empty())
        {
          cout << "Must specify a filename" << endl;
        }
        else
        {
          if(!loadDna(args))
          {
            cout << "Loaded " << args << endl;
          }
          else
          {
            cout << "Error loading dna" << endl;
          }
        }
      }
    } else
    if(command=="exit")
    {
      stopThreads();
      exit(EXIT_SUCCESS);
    } else
    {
      cout << "What?" << endl;
    }
  }
};
