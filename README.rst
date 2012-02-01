==========
Fcitx-m17n
==========

This is the source tree of fcitx-m17n, some trivial little codes the wrap the
m17n input method engine into a fcitx input method.

Tree
====
* im:
  Most codes.
* po:
  Translation files suitable for GNU gettext.
* testmim:
  A trivial standalone frontend to the m17n input methods engine.

Dependency, Building and Installation
=====================================
Fcitx-m17n is maintained using CMake

To install, run::

  mkdir build; cd build; cmake -DCMAKE_INSTALL_PREFIX=/usr; make; make install

Dependency::

  +---------+------------+------------+
  | subdir  |   im & po  | testmim    |
  +---------+------------+------------+
  | runtime |   fcitx    |            |
  |         |            |            |
  |         +------------+------------+
  |         |        libm17n          |
  +---------+------------+------------+
  |  build  | pkg-config |   clang    |
  |  extra  |  intltool  |            |
  |         |   gettext  |            |
  +---------+------------+------------+

License
=======
All codes are currently licensed in the LGPL 2.1.
See `COPYING` for a copy of the license. A list of of fcitx-m17n
authors and contributors is included herein:

* Cheer Xiao
* CSSlayer
