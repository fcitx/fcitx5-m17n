/*
 * Copyright (C) 2012-2012 Cheer Xiao
 * Copyright (C) 2012-2012 CSSlayer
 *
 * This library is free software; you can redistribute it and/or
 * modify it under the terms of the GNU Lesser General Public
 * License as published by the Free Software Foundation; either
 * version 2.1 of the License, or (at your option) any later version.
 *
 * This library is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the GNU
 * Lesser General Public License for more details.
 *
 * You should have received a copy of the GNU Lesser General Public
 * License along with this library; if not, write to the
 * Free Software Foundation, Inc.
 * 51 Franklin St, Fifth Floor, Boston, MA 02110-1301, USA.
 */

#ifndef __FCITX_M17N_H
#define __FCITX_M17N_H

#include <m17n.h>
#include <fcitx-config/fcitx-config.h>
#include <fcitx-config/hotkey.h>
#include <fcitx/instance.h>

typedef INPUT_RETURN_VALUE FcitxIRV;
struct _FcitxInstance;

typedef struct _FcitxM17NConfig
{
    FcitxGenericConfig gconfig;
    FcitxHotkey hkPrevPage[2];
    FcitxHotkey hkNextPage[2];
    boolean enableDeprecated;
} FcitxM17NConfig;

struct _Addon;
typedef struct {
    struct _Addon* owner;
    boolean forward;
    MSymbol mname;
    MSymbol mlang;
    int pageSize;
} IM;

typedef struct _Addon {
    struct _FcitxInstance* owner;
    FcitxM17NConfig config;
    size_t nim;
    IM** ims;
    MInputMethod* mim;
    MInputContext* mic;
} Addon;

CONFIG_BINDING_DECLARE(FcitxM17NConfig);

#endif
