frameserver
===========

A multiprocess frameserver and plugin framework.

## Requirements
  * ffmpeg 2.1
  
## Building on Linux
  This project is built with a cross compiling gcc toolchain in Linux, using the spank build system, available at https://github.com/noname22/spank.
	
    spank build
  or

    spank build release 

## Building with Docker
    docker build -t fsbuild docker
    docker run --rm -i -t -v $(pwd):/root/build/source fsbuild
