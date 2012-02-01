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
  NOTE: po/Makefile is incomplete at the moment.
* testmim:
  A trivial standalone frontend to the m17n input methods engine.

Dependency, Building and Installation
=====================================
Fcitx-m17n is maintained using GNU Make. Configuration is done by editing
`config.mk`; subdirectory `testmim` also has its own `config.mk`.

To install, run::

  cd im; make && sudo make install

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
All codes are currently licensed in the 2-clause BSD license (simplified BSD
license).  See `COPYING` for a copy of the license. A list of of fcitx-m17n
authors and contributors is included herein:

* Cheer Xiao

