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

#include "fcitx-m17n.h"

CONFIG_BINDING_BEGIN(FcitxM17NConfig)
CONFIG_BINDING_REGISTER("M17N", "PrevPage", hkPrevPage)
CONFIG_BINDING_REGISTER("M17N", "NextPage", hkNextPage)
CONFIG_BINDING_REGISTER("M17N", "EnableDeprecated", enableDeprecated)
CONFIG_BINDING_END()
