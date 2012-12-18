/* -*- mode: C; c-basic-offset: 4; indent-tabs-mode: nil; -*- */
/* GDK - The GIMP Drawing Kit
* Copyright (C) 1995-1997 Peter Mattis, Spencer Kimball and Josh MacDonald
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

/*
* Modified by the GTK+ Team and others 1997-2000. See the AUTHORS
* file for a list of people on the GTK+ Team. See the ChangeLog
* files for a list of changes. These files are distributed with
* GTK+ at ftp://ftp.gtk.org/pub/gtk/.
*/

#include <fcitx-config/hotkey.h>

#include "keyname.h"
#include "keysymname.h"

#define NUM_KEYS (sizeof(gdk_keys_by_keyval) / sizeof(gdk_keys_by_keyval[0]))

static int
gdk_keys_keyval_compare (const void *pkey, const void *pbase)
{
    return (*(int *) pkey) - ((gdk_key *) pbase)->keyval;
}

const char*
KeySymName (FcitxKeySym keyval)
{
    static char buf[100];
    gdk_key *found;

    /* Check for directly encoded 24-bit UCS characters: */
    if ((keyval & 0xff000000) == 0x01000000)
    {
        sprintf (buf, "U+%.04X", (keyval & 0x00ffffff));
        return buf;
    }

    found = bsearch (&keyval, gdk_keys_by_keyval,
                     NUM_KEYS, sizeof (gdk_key),
                     gdk_keys_keyval_compare);

    if (found != NULL)
    {
        while ((found > gdk_keys_by_keyval) &&
                ((found - 1)->keyval == keyval))
            found--;

        return (char *) (keynames + found->offset);
    }
    else if (keyval != 0)
    {
        sprintf (buf, "%#x", keyval);
        return buf;
    }

    return NULL;
}

