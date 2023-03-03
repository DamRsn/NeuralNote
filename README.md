# JUCE CMake Repo Prototype
A prototype to model a way to create an entire repo using JUCE 6 and CMake.

This is inspired by a desire to keep the environment setting of my projects to minimum,
making sure the environment is identical for every developer/machine.

The main concept is to set all the different variables (where JUCE is, custom modules, etc) 
in the top CMakeLists.txt, then add all your projects with very little setup time.

Another important concept is to share all 'related' projects under the same configuration,
which I prefer, since it encourages code-sharing and build system settings sharing.
In some of the examples I added minimal usages of juce-style modules to illustrate how that
can be done.

To build, all you have to do is load this project in your favorite IDE 
(CLion/Visual Studio/VSCode/etc) 
and click 'build' for one of the targets (templates, JUCE examples, Projucer, etc).

You can also generate a project for an IDE by using (Mac):
```
cmake -G Xcode -B build
```
Windows:
```
cmake -G "Visual Studio 16 2022" -B build
```

For package management, I'm using the amazing CPM.cmake:
#https://github.com/TheLartians/CPM.cmake
It automatically fetches JUCE from git, but you can also set the variable:
CPM_JUCE_SOURCE to point it to a local folder, by using:
``-DCPM_JUCE_SOURCE="Path_To_JUCE"``
when invoking CMake

JUCE can be found here:
#https://github.com/juce-framework/JUCE

License:
Anything from me in this repo is completely free to use for any purpose. 
However, please check the licenses for CPM and JUCE as described in their repo. 
