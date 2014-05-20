# Distvolve #

A lightweight C++ Image evolver.

### Installation ###
There are no binarys available at this time. It is reccomended to build this project with optimizations for your CPU architecture.

### Features ###

- Multithreaded
- Polygon size hinting (Smaller polys when generation count is high)
- Lightweight
- GUIless
- PNG support

### TODO ###

- Work distribution
- Load more than a PNG
- DNA loading / saving
- SVG saving
- GUI

### Commands ###

Start the executable.

``` $ l image.png ``` - Loads the ```image.png```

``` $ b 8 ``` - Start ```8``` threads

``` $ s output.png ``` - Save the current polygons to ```output.png```.

``` $ c ``` - Stop all of the worker threads but do not quit.

``` $ p ``` - Prints the current number of polygons and generations.

### Building ###

Building requires a C++11 compiler with pthreads support. I reccomend [mingw builds](http://sourceforge.net/projects/mingwbuilds/).
This can be compiled in Release configuration in Code::Blocks with the included distvolve.cbp
Otherwise you can compile with
```
g++ -Wall -fexceptions -O3 -std=c++11 -march=native -I. -c *.cpp
```

### Issues ###
If you find as issue you can report it on the [bugtracker](https://bitbucket.org/Abex/distvolve/issues?status=new&status=open)