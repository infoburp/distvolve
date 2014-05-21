#include <iostream>
#include <cstdlib>
#include <lodepng.h>
#include <Polygon.hpp>
#include <ThreadWorker.hpp>
#include <string>
#include <mutex>
#include <atomic>
#include <fstream>
#include <algorithm>
#include <thread>
#include <queue>
#include <sstream>

#ifdef _WIN32
  #include<windows.h>
#endif // _WIN32

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
atomic<int> saveOn(0);
mutex mArgumentCommands;
queue<string> argumentCommands;

struct event {
  int number;
  string command;
  bool modulo;
  int lastRun;
};

 vector <event>polyEvents;
 vector <event>generationEvents;

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
    distvolve::Polygon newPoly(width,height,filestream);
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

void cStrToArgs(string argvs,bool pop =false)
{ // if someone wants to fix this shit just send a pull /r/
  //TODO Fix this shit
  queue<string> argv;
  string current="";
  for(unsigned c=0;c<argvs.length();c++)
  {
    char ch = argvs[c];
    char carray[2] = " ";
    if(ch=='\"')
    {
      carray[0]=ch;
      current.append(carray);
      ch = argvs[++c];
      while(ch!='"')
      {
        carray[0]=ch;
        current.append(carray);
        ch = argvs[++c];
      }
    }
    else if(ch==' ')
    {
      argv.push(current);
      current="";
    }
    else
    {
      carray[0]=ch;
      current.append(carray);
    }
  }
  argv.push(current);
  if(pop)
  {
    argv.pop();
    argv.pop();
  }
  string command="";
  while(!argv.empty())
  {
    string arg=argv.front();
    argv.pop();
    if(arg[0]=='-')
    {
      if(command!="")
      {
        argumentCommands.push(command);
      }
      command = arg.substr(1);
    }
    else
    {
      command.append(" ");
      command.append(arg);
    }
  }
  argumentCommands.push(command);
}

int main(int argc, char **argv)
{
  time_t nTime=0;
  if(argc!=1)
  {
    #ifndef _WIN32
      stringstream sstr;
      for(int i=1;i<argc;i++)
      {
        sstr << argv[i] << " ";
      }
      cStrToArgs(sstr.str(),true);
    #else
      cStrToArgs(string(GetCommandLine()),true);
    #endif
  }
  thread inputThread([&mArgumentCommands,&argumentCommands]()
  {
    while(true)
    {
      char cStr[256];
      cin.getline(cStr,256);
      mArgumentCommands.lock();
      argumentCommands.push(string(cStr));
      mArgumentCommands.unlock();
    }
  });
  inputThread.detach();
  string endl = "\n(dsv)$ ";
  cout << "DISTVOLVE V0.1.2" << endl;
  while(true)
  {
    string line;
    mArgumentCommands.lock();
    if(argumentCommands.empty())
    {
      mArgumentCommands.unlock();
      if(nTime<time(NULL))
      {
        for(event &e:polyEvents)
        {
          if(e.modulo)
          {
            int pModulo = polygons%e.number;
            if(pModulo<e.lastRun)
            {
              cStrToArgs(e.command); //run
            }
            e.lastRun = pModulo;
          }
          else
          {
            if(e.number<polygons && e.lastRun<e.number)
            {
              e.lastRun=polygons;
              cStrToArgs(e.command);
            }
          }
        }
        for(event &e:generationEvents)
        {
          if(e.modulo)
          {
            int pModulo = generations%e.number;
            if(pModulo<e.lastRun)
            {
              cStrToArgs(e.command); //run
            }
            e.lastRun = pModulo;
          }
          else
          {
            if(e.number<generations && e.lastRun<e.number)
            {
              e.lastRun=generations;
              cStrToArgs(e.command);
            }
          }
        }
        nTime =time(NULL)+1;
      }
      this_thread::yield();
      continue;
    }
    else
    {
      line = argumentCommands.front();
      argumentCommands.pop();
      mArgumentCommands.unlock();
    }
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
        size_t locPolys = args.find("#polys#");
        if(locPolys!=string::npos)
        {
          char cpolys[16];
          sprintf(cpolys,"%d",polygons.load());
          args.replace(locPolys,7,cpolys);
        }
        size_t locGens = args.find("#gens#");
        if(locGens!=string::npos)
        {
          char cgens[16];
          sprintf(cgens,"%d",generations.load());
          args.replace(locGens,7,cgens);
        }
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
    if(command=="execute"||command=="e")
    {
      event e;
      if(args[0]=='g' || args[0]=='p')
      {
        e.modulo = args[2]=='%';
        int i=2;
        if(e.modulo)
          i=4;
        e.number = atoi(args.substr(i,args.substr(i).find(" ")+i).c_str());
        e.command = args.substr(args.substr(i).find(" ")+i+2);
        if(args[0]=='p')
        {
          e.lastRun = polygons%e.number;
          polyEvents.push_back(e);
        }
        else
        {
          generationEvents.push_back(e);
        }//yaoi horses ... forces
        cout << "Added event" << endl;
      }
      else
        cout << "e: Syntax error" << endl;
    }else
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
