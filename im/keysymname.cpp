/*
 * SPDX-FileCopyrightText: 2017~2017 CSSlayer <wengxt@gmail.com>
 *
 * SPDX-License-Identifier: LGPL-2.1-or-later
 *
 */

#include "keysymname.h"
#include <cstdio>
#include <fcitx-utils/key.h>
#include <fcitx-utils/keysym.h>
#include <string>

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
