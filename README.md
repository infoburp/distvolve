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

- Proper Fitness
- automatic detal stop
- max poly count
- poly optimization
- Work distribution
- Load more than a PNG
- DNA loading / saving
- SVG saving
- GUI
- Commented code

### Commands ###

These can be entered in the interactive prompt or as a argument by prefixing with a ``` - ```

Command line usage

``` distvolve.exe -l image.png -b 8 

``` (dsv)$ l image.png ``` - Loads the ```image.png```

``` (dsv)$ b 8 ``` - Start ```8``` threads

``` (dsv)$ s output.png ``` - Save the current polygons to ```output.png```.

``` (dsv)$ s #polys#.png ``` - Save the current polygons with the number of polys in its name. 
This also works with ``` #gens# ``` 

``` (dsv)$ s ``` - Save the current polygons to the last saved file.

``` (dsv)$ c ``` - Stop all of the worker threads but do not quit.

``` (dsv)$ p ``` - Prints the current number of polygons and generations.

```  e <g|p> [%] <value> <command>```

``` (dsv)$ e p 8000 "-s output.png -exit" ``` - Saves to ```output.png``` and exits at 8000 polygons

To stop generating and quit after 1000 polys.

``` distvolve.exe -l image.png -b 8 -e p 1000 "-c -s imageOut.png -exit"```

### Building ###

Building requires a C++11 compiler with pthreads support. I reccomend [mingw builds](http://sourceforge.net/projects/mingwbuilds/).
This can be compiled in Release configuration in Code::Blocks with the included distvolve.cbp
Otherwise you can compile with
```
g++ -Wall -fexceptions -O3 -std=c++11 -march=native -I. *.cpp
```

### Issues ###
If you find as issue you can report it on the [bugtracker](https://bitbucket.org/Abex/distvolve/issues?status=new&status=open)