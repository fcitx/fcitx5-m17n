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
#include <fcitx/candidate.h>

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

// Don't use this for large indices or (worse) list iteration.
void *mplistSub(MPlist *head, size_t idx) {
    while (idx--) {
        head = mplist_next(head);
    }
    return mplist_value(head);
}

inline static void setPreedit(FcitxInputState* is, const char* s)
{
    FcitxMessages* m = FcitxInputStateGetClientPreedit(is);
    FcitxMessagesSetMessageCount(m, 0);
    FcitxMessagesAddMessageAtLast(m, MSG_INPUT, "%s", s);
}

INPUT_RETURN_VALUE FcitxM17NGetCandWord(void *arg, FcitxCandidateWord *cand) {
    return IRV_DO_NOTHING;
}

INPUT_RETURN_VALUE FcitxM17NGetCandWords(void *arg)
{
    // FcitxLog(INFO, "CandWords");
    IM* im = (IM*) arg;
    FcitxInstance* inst = im->owner->owner;
    FcitxInputState* is = FcitxInstanceGetInputState(inst);
    // FcitxInputContext* ic = FcitxInstanceGetCurrentIC(inst);
    FcitxCandidateWordList *cl = FcitxInputStateGetCandidateList(is);
    // FcitxCandidateWordReset(cl);
    if (im->mic->candidate_show) {
        FcitxLog(INFO, "Building candidate list");
        FcitxCandidateWord cand;
        cand.owner = im;
        cand.callback = FcitxM17NGetCandWord;
        cand.priv = NULL;
        cand.strExtra = NULL;
        cand.wordType = MSG_OTHER;

        MPlist *head = im->mic->candidate_list;
        for (; head; head = mplist_next(head)) {
            MSymbol key = mplist_key(head);
            if (key == Mplist) {
                MPlist *head2 = mplist_value(head);
                for (; head2; head2 = mplist_next(head2)) {
                    MText *word = mplist_value(head2);
                    // Fcitx will do the free() for us.
                    cand.strWord = mtextToUTF8(word);
                    m17n_object_unref(word);
                    FcitxCandidateWordAppend(cl, &cand);
                }
            } else if (key == Mtext) {
                char *words = mtextToUTF8((MText*) mplist_value(head));
                char *p, *q;
                for (p = words; *p; p = q) {
                    int unused;
                    q = fcitx_utf8_get_char(p, &unused);
                    cand.strWord = strndup(p, q-p);
                    FcitxCandidateWordAppend(cl, &cand);
                }
                free(words);
            } else {
                FcitxLog(INFO, "%s", msymbol_name(key));
            }
        }
    }
    // return IRV_DO_NOTHING;
    return IRV_DISPLAY_CANDWORDS;
    // return IRV_FLAG_UPDATE_INPUT_WINDOW;
}

MSymbol FcitxM17NKeySymToSymbol (FcitxKeySym sym, unsigned int state)
{
    /*
     Rationale:

     This function converts fcitx key symbols to m17n key symbols.

     Both fcitx and m17n uses X symbols to represent basic key events
     (without modifiers). The conversion of modifier syntax is implemented in
     this function.

     The formalism provided by fcitx intended for more portable code, like
     enums such as FCITX_BACKSPACE to represent Backspace key event, macros
     such as FcitxHotkeyIsHotkey to do comaparision, is largely disregarded
     here. Similiar situation for libm17n's notion of special keys like
     msymbol("BackSpace") for Backspace key event.

     MSymbol's don't need to be finalized in any way.
     */

    MSymbol mkeysym = Mnil;
    unsigned int mask = 0;

    if (sym >= FcitxKey_Shift_L && sym <= FcitxKey_Hyper_R) {
        return Mnil;
    }

    const char* base = "";

    if (sym >= FcitxKey_space && sym <= FcitxKey_asciitilde) {
        FcitxKeySym c = sym;

        if (sym == FcitxKey_space && (state & FcitxKeyState_Shift))
            mask |= FcitxKeyState_Shift;

        if (state & FcitxKeyState_Ctrl) {
            if (c >= FcitxKey_a && c <= FcitxKey_z)
                c += FcitxKey_A - FcitxKey_a;
            mask |= FcitxKeyState_Ctrl;
        }

        char temp[2] = " ";
        temp[0] = c & 0xff;
        base = temp;
    }
    else {
        mask |= state & (FcitxKeyState_Ctrl_Shift);
        base = KeySymName(sym);
        if (base == NULL || strlen(base) == 0) {
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
    // This is mysterious. - xiaq
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
    asprintf(&keystr, "%s%s", prefix, base);

    mkeysym = msymbol(keystr);
    free(keystr);

    return mkeysym;
}

INPUT_RETURN_VALUE FcitxM17NDoInput(void* arg, FcitxKeySym sym, unsigned state)
{
    /*
     This is what Fcitx calls for each event it receives.

     The "return value"-styled interaction is, to some extent, disregarded
     here since it is tedious to instruct multiple operations during one
     DoInput call; instead, Fcitx operating functions like
     FcitxInstanceUpdatePreedit are employed.

     Thus, most of the time DoInput simply returns IRV_DO_NOTHING to suppress
     further Fcitx-side processing, except when the symbol was let through by
     m17n (IRV_TO_PROCESS) or the candidate list needs to be shown
     (IRV_DISPLAY_CANDS).
     */

    // FcitxLog(INFO, "DoInput got sym=%x, state=%x, hahaha", sym, state);
    MSymbol msym = FcitxM17NKeySymToSymbol(sym, state);

    if (msym == Mnil) {
        FcitxLog(INFO, "sym=%x, state=%x, not my dish", sym, state);
        return IRV_TO_PROCESS;
    }

    IM* im = (IM*) arg;
    FcitxInstance* inst = im->owner->owner;
    FcitxInputState* is = FcitxInstanceGetInputState(inst);
    FcitxInputContext* ic = FcitxInstanceGetCurrentIC(inst);

    int thru = 0;
    if (!minput_filter(im->mic, msym, NULL)) {
        MText* produced = mtext();
        // If input symbol was let through by m17n, let Fcitx handle it.
        // m17n may still produce some text to commit, though.
        thru = minput_lookup(im->mic, msym, NULL, produced);
        if (mtext_len(produced) > 0) {
            char* buf = mtextToUTF8(produced);
            FcitxInstanceCommitString(inst, ic, buf);
            free(buf);
        }
        m17n_object_unref(produced);
    }

    if (im->mic->preedit_changed || im->mic->cursor_pos_changed) {
        char* preedit = mtextToUTF8(im->mic->preedit);
        FcitxLog(INFO, "preedit is %s", preedit);
        setPreedit(is, preedit);
        FcitxInputStateSetClientCursorPos(is, 
            fcitx_utf8_get_nth_char(preedit, im->mic->cursor_pos) - preedit);

        FcitxInstanceUpdatePreedit(inst, ic);
        free(preedit);
    }

    if (im->mic->status_changed) {
        char* mstatus = mtextToUTF8(im->mic->status);
        // TODO Fcitx should be able to show the status in some way.
        FcitxLog(INFO, "IM status changed to %s", mstatus);
        free(mstatus);
    }

    if (thru) {
        return IRV_TO_PROCESS;
    } else if (im->mic->candidates_changed) {
        return IRV_FLAG_UPDATE_CANDIDATE_WORDS;
        // return IRV_DISPLAY_CANDWORDS;
    } else {
        return IRV_DO_NOTHING;
    }
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

IM* makeIM(Addon* owner, MSymbol mlang, MSymbol mname)
{
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

void *FcitxM17NCreate(FcitxInstance* inst)
{
    bindtextdomain(TEXTDOMAIN, LOCALEDIR);
    M17N_INIT();

    Addon* addon = (Addon*) fcitx_utils_malloc0(sizeof(Addon));
    addon->owner = inst;

    MPlist *mimlist = minput_list(Mnil);
    addon->nim = mplist_length(mimlist);
    // This wastes some space if some of the m17n IM's are not "sane",
    // but it also makes the the code a little simpler.
    addon->ims = (IM**) fcitx_utils_malloc0(sizeof(IM*) * addon->nim);

    int i;
    for (i = 0; i < addon->nim; i++, mimlist = mplist_next(mimlist)) {
        // See m17n documentation of minput_list() in input.c.
        MPlist *info;
        info = (MPlist*) mplist_value(mimlist);
        MSymbol mlang = (MSymbol) mplistSub(info, 0);
        MSymbol mname = (MSymbol) mplistSub(info, 1);
        MSymbol msane = (MSymbol) mplistSub(info, 2);

        char *lang = msymbol_name(mlang);
        char *name = msymbol_name(mname);

        if (msane != Mt) {
            // Not "sane"
            FcitxLog(WARNING, "Insane IM [%s: %s]", lang, name);
            continue;
        }
        if (!(addon->ims[i] = makeIM(addon, mlang, mname))) {
            FcitxLog(ERROR, "Failed to create IM [%s: %s]", lang, name);
            continue;
        }
        FcitxLog(INFO, "Created IM [%s: %s]", lang, name);

        char *uniqueName, *fxName, *iconName;
        asprintf(&uniqueName, "m17n_%s_%s", lang, name);
        asprintf(&fxName, _("%s - %s (M17N)"), lang, name);

        info = minput_get_title_icon(mlang, mname);
        // head of info is a MText
        m17n_object_unref(mplistSub(info, 0));
        MText *iconPath = (MText*) mplistSub(info, 1);

        if (iconPath) {
            // This disregards the fact that the path to m17n icon files
            // might have utf8-incompatible path strings. However, since the
            // path almost always consists of ascii characters (typically
            // /usr/share/m17n/icons/... on Linux systems, this is a
            // reasonable assumption.
            iconName = mtextToUTF8(iconPath);
            m17n_object_unref(iconPath);
            FcitxLog(INFO, "Mim icon is %s", iconName);
        } else {
            iconName = uniqueName;
        }

        FcitxInstanceRegisterIM(
            inst, addon->ims[i],
            uniqueName, fxName, iconName,
            FcitxM17NInit, FcitxM17NReset, FcitxM17NDoInput,
            FcitxM17NGetCandWords, NULL, FcitxM17NSave,
            FcitxM17NReload, NULL,
            100, strcmp(lang, "t") == 0 ? "*" : lang
        );
        if (iconName != uniqueName) {
            free(iconName);
        }
        free(uniqueName);
        free(fxName);
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

