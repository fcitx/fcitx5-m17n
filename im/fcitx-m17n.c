/* 
 * Copyright (C) 2012-2012 Cheer Xiao
 * Copyright (C) 2012-2012 CSSlayer
 *
 * This library is free software; you can redistribute it and/or
 * modify it under the terms of the GNU Lesser General Public
 * License as published by the Free Software Foundation; either
 * version 2 of the License, or (at your option) any later version.
 *
 * This library is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the GNU
 * Lesser General Public License for more details.
 *
 * You should have received a copy of the GNU Lesser General Public
 * License along with this library; if not, write to the
 * Free Software Foundation, Inc., 59 Temple Place - Suite 330,
 * Boston, MA 02111-1307, USA.
 */


#include <string.h>
#include <stdlib.h>
#include <stdio.h>
#include <libintl.h>
#include <locale.h>
#include <m17n.h>
#include <fcitx-utils/log.h>
#include <fcitx-utils/utf8.h>
#include <fcitx/keys.h>
#include <fcitx/instance.h>
#include <fcitx/context.h>

#include "fcitx-m17n.h"

#define TEXTDOMAIN "fcitx-m17n"
#define _(x) dgettext(TEXTDOMAIN, x)

FCITX_EXPORT_API
FcitxIMClass ime = {
    FcitxM17NCreate,
    FcitxM17NDestroy
};

FCITX_EXPORT_API
int ABI_VERSION = FCITX_ABI_VERSION;

const char*
KeySymName (FcitxKeySym keyval);

char* mtextToUTF8(MText* mt)
{
    // TODO Verify that bufsize is "just enough" in worst scenerio.
    size_t bufsize = (mtext_len(mt) + 1) * UTF8_MAX_LENGTH;
    char* buf = (char*) fcitx_utils_malloc0(bufsize);

    MConverter* mconv = mconv_buffer_converter(Mcoding_utf_8, (unsigned char*) buf, bufsize);
    mconv_encode(mconv, mt);
    mconv_free_converter(mconv);

    buf[mconv->nbytes] = '\0';
    return buf;
}

inline static void setPreedit(FcitxInputState* is, const char* s)
{
    FcitxMessages* m = FcitxInputStateGetClientPreedit(is);
    FcitxMessagesSetMessageCount(m, 0);
    FcitxMessagesAddMessageAtLast(m, MSG_INPUT, "%s", s);
}

// Never called since FcitxM17NDoInput never returns IRV_SHOW_CANDWORDS.
INPUT_RETURN_VALUE FcitxM17NGetCandWords(void *arg)
{
    return IRV_CLEAN;
}

MSymbol
FcitxM17NKeySymToSymbol (FcitxKeySym sym, unsigned int state)
{
    MSymbol mkeysym = Mnil;
    unsigned int mask = 0;

    if (sym >= FcitxKey_Shift_L && sym <= FcitxKey_Hyper_R) {
        return Mnil;
    }
    
    char temp[2] = {'\0', '\0'};
    const char* center = "";

    if (sym >= FcitxKey_space && sym <= FcitxKey_asciitilde) {
        FcitxKeySym c = sym;

        if (sym == FcitxKey_space && (state & FcitxKeyState_Shift))
            mask |= FcitxKeyState_Shift;

        if (state & FcitxKeyState_Ctrl) {
            if (c >= FcitxKey_a && c <= FcitxKey_z)
                c += FcitxKey_A - FcitxKey_a;
            mask |= FcitxKeyState_Ctrl;
        }
       
       temp[0] = c & 0xff;
       center = temp;
    }
    else {
        mask |= state & (FcitxKeyState_Ctrl_Shift);
        center =  KeySymName (sym);
        if (center == NULL || strlen(center) == 0) {
            return Mnil;
        }
    }

    mask |= state & (FcitxKeyState_UsedMask);

    const char* prefix = "";

    if (mask & FcitxKeyState_Hyper) {
        prefix = "H-";
    }
    if (mask & FcitxKeyState_Super2) {
        prefix = "s-";
    }
    if (mask & FcitxKeyState_ScrollLock) {
        prefix = "G-";
    }
    if (mask & FcitxKeyState_Alt) {
        prefix = "A-";
    }
    if (mask & FcitxKeyState_Meta) {
        prefix = "M-";
    }
    if (mask & FcitxKeyState_Ctrl) {
        prefix = "C-";
    }
    if (mask & FcitxKeyState_Shift) {
        prefix = "S-";
    }
    
    char* keystr;
    asprintf(&keystr, "%s%s", prefix, center);

    mkeysym = msymbol (keystr);
    free(keystr);

    return mkeysym;
}

INPUT_RETURN_VALUE FcitxM17NDoInput(void* arg, FcitxKeySym sym, unsigned state)
{
    // FcitxLog(INFO, "DoInput got sym=%x, state=%x, hahaha", sym, state);
    /*
     Rationale:

     This code first tries to map special keys from fcitx convention to
     libm17n convention.

     fcitx uses X symbols convention, but it also provides a huge bunch of
     enums like FCITX_BACKSPACE plus a macro FcitxHotkeyIsHotKey, which is
     supposed to be cleaner than using the X primitives directly.

     libm17n's notion of special keys is not yet clear to me. For one thing,
     MSymbol is used as the universal struct for all "atoms" in libm17n, and
     it is known that msymbol("BackSpace") is the MSymbol for obviously,
     backspace.

     MSymbol's don't need to be finalized in any way.
     */
    MSymbol msym = FcitxM17NKeySymToSymbol(sym, state);
    
    if (msym == Mnil) {
        FcitxLog(INFO, "sym=%x, state=%x, not my dish", sym, state);
        return IRV_TO_PROCESS;
    }

    IM* im = (IM*) arg;
    FcitxInstance* inst = im->owner->owner;
    FcitxInputState* is = FcitxInstanceGetInputState(inst);
    FcitxInputContext* ic = FcitxInstanceGetCurrentIC(inst);

    // Input symbol was passed "through" by m17n
    int thru = 0;
    if (!minput_filter(im->mic, msym, NULL)) {
        MText* produced = mtext();
        thru = minput_lookup(im->mic, msym, NULL, produced);
        if (mtext_len(produced) > 0) {
            char* buf = mtextToUTF8(produced);
            FcitxInstanceCommitString(inst, ic, buf);
            free(buf);
        }
        m17n_object_unref(produced);
    }

    char* preedit = mtextToUTF8(im->mic->preedit);
    setPreedit(is, preedit);
    FcitxInputStateSetClientCursorPos(is, fcitx_utf8_get_nth_char(preedit, im->mic->cursor_pos) - preedit);

    FcitxInstanceUpdatePreedit(inst, ic);
    free(preedit);

    char* mstatus = mtextToUTF8(im->mic->status);
    FcitxLog(INFO, "mic status is %s", mstatus);
    free(mstatus);

    return thru ? IRV_TO_PROCESS : IRV_DO_NOTHING;
}

void FcitxM17NReset(void *arg)
{
    // FcitxLog(INFO, "Reset called, hahaha");
    IM* im = (IM*) arg;
    minput_reset_ic(im->mic);
}

boolean FcitxM17NInit(void *arg)
{
    // FcitxLog(INFO, "Init called, hahaha");
    IM* im = (IM*) arg;
    FcitxInstance* inst = im->owner->owner;
    boolean flag = true;
    FcitxInstanceSetContext(inst, CONTEXT_DISABLE_AUTOENG, &flag);
    FcitxInstanceSetContext(inst, CONTEXT_DISABLE_QUICKPHRASE, &flag);
    return true;
}

void FcitxM17NReload(void *arg)
{
    // FcitxLog(INFO, "Reload called, hahaha");
}

void FcitxM17NSave(void *arg)
{
    // FcitxLog(INFO, "Save called, hahaha");
}

IM* makeIM(Addon* owner, const char* lang, const char* name)
{
    MSymbol mlang = msymbol(lang);
    if (!mlang) return NULL;
    MSymbol mname = msymbol(name);
    if (!mname) return NULL;
    MInputMethod *mim = minput_open_im(mlang, mname, NULL);
    if (!mim) return NULL;
    MInputContext *mic = minput_create_ic(mim, NULL);
    if (!mic) {
        minput_close_im(mim);
        return NULL;
    }
    IM *im = (IM*) fcitx_utils_malloc0(sizeof(IM));
    im->owner = owner;
    im->mim = mim;
    im->mic = mic;
    // FcitxLog(INFO, "IM %s(%s) created, hahaha", name, lang);
    return im;
}

void delIM(IM* im)
{
    minput_destroy_ic(im->mic);
    minput_close_im(im->mim);
    free(im);
}

typedef struct {
    const char* lang;
    const char* name;
} m17nIM;

static m17nIM m17nIMs[] = {
    {"t", "latn-post"},
    {"el", "kbd"},
    {"eo", "plena"},
    {"ja", "anthy"},
    {NULL, NULL}
};

void *FcitxM17NCreate(FcitxInstance* inst)
{
    bindtextdomain(TEXTDOMAIN, LOCALEDIR);
    M17N_INIT();

    Addon* addon = (Addon*) fcitx_utils_malloc0(sizeof(Addon));
    addon->owner = inst;

    // TODO Built-in list of IMs only.
    addon->nim = 0;
    m17nIM* p;
    for (p = m17nIMs; p->lang; p++) {
        addon->nim++;
    }

    addon->ims = (IM**) fcitx_utils_malloc0(sizeof(IM*) * addon->nim);

    int i = 0;
    for (p = m17nIMs; p->lang; p++, i++) {
        if (!(addon->ims[i] = makeIM(addon, p->lang, p->name))) {
            FcitxLog(ERROR, "Failed to create IM [%s: %s]", p->lang, p->name);
            continue;
        }
        char *uniqueName, *name, *iconName;
        asprintf(&uniqueName, "m17n_%s_%s", p->lang, p->name);
        asprintf(&name, _("%s - %s (M17N)"), p->lang, p->name);
        iconName = uniqueName;
        FcitxInstanceRegisterIM(
            inst, addon->ims[i],
            uniqueName, name, iconName,
            FcitxM17NInit, FcitxM17NReset, FcitxM17NDoInput,
            FcitxM17NGetCandWords, NULL, FcitxM17NSave,
            FcitxM17NReload, NULL,
            100, strcmp(p->lang, "t") == 0 ? NULL : p->lang
        );
        free(uniqueName);
        free(name);
    }

    return addon;
}

void FcitxM17NDestroy(void *arg)
{
    Addon* addon = (Addon*) arg;
    int i;
    for (i = 0; i < addon->nim; i++) {
        if (addon->ims[i]) {
            delIM(addon->ims[i]);
        }
    }
    free(addon);
    M17N_FINI();
}

