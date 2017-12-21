//
// Copyright (C) 2017~2017 by CSSlayer
// wengxt@gmail.com
//
// This library is free software; you can redistribute it and/or modify
// it under the terms of the GNU Lesser General Public License as
// published by the Free Software Foundation; either version 2.1 of the
// License, or (at your option) any later version.
//
// This library is distributed in the hope that it will be useful,
// but WITHOUT ANY WARRANTY; without even the implied warranty of
// MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the GNU
// Lesser General Public License for more details.
//
// You should have received a copy of the GNU Lesser General Public
// License along with this library; see the file COPYING. If not,
// see <http://www.gnu.org/licenses/>.
//

#include "keysymname.h"

namespace fcitx {
std::string KeySymName(KeySym keyval) {
    char buf[100];

    /* Check for directly encoded 24-bit UCS characters: */
    if ((keyval & 0xff000000) == 0x01000000) {
        sprintf(buf, "U+%.04X", (keyval & 0x00ffffff));
        return buf;
    }

    auto name = Key::keySymToString(keyval);
    if (name.empty() && keyval != FcitxKey_None) {
        sprintf(buf, "%#x", keyval);
        return buf;
    }
    return name;
}

} // namespace fcitx
