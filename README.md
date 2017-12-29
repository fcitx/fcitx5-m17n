Fcitx-m17n
========

This is the source tree of fcitx-m17n, some trivial little codes the wrap the
m17n input method engine into a fcitx input method.


[![Jenkins Build Status](https://img.shields.io/jenkins/s/https/jenkins.fcitx-im.org/job/fcitx5-m17n.svg)](https://jenkins.fcitx-im.org/job/fcitx5-m17n/)

[![Coverity Scan Status](https://img.shields.io/coverity/scan/14675.svg)](https://scan.coverity.com/projects/fcitx-fcitx5-m17n)


Source Tree
===========

* im:
  Most codes.
* po:
  Translation files suitable for GNU gettext.
* testmim:
  A trivial standalone frontend to the m17n input methods engine.

Dependency, Building and Installation
=====================================

Fcitx-m17n is maintained using CMake. To install, run::

```
  mkdir build; cd build
  cmake .. [ FLAGS ] 
  make && make install
```

To install to /usr, include ``-DCMAKE_INSTALL_PREFIX=/usr`` in ``FLAGS``. To
build with debug symbols, include ``-DCMAKE_BUILD_TYPE``. You may as well
replace ``cmake`` with ``ccmake``, which gives you a nice curses UI to adjust
CMake flags.

Dependencies is handled by CMake and therefore not listed here. But note that
testmim uses some C++11 features and requires a recent C++ compiler. Latest
clang will suffice.

You may want to read ``CMakeLists.txt`` in the subdirectories.

License
=======

All codes are currently licensed in LGPL 2.1+.

See ``COPYING`` for a copy of the license. A list of of fcitx-m17n authors and
contributors is included herein:

* Cheer Xiao
* CSSlayer

