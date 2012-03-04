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

#include "config.h"

#ifdef NEED_MINPUT_LIST
#include "minput_list.h"
#endif

#include <string.h>
#include <stdlib.h>
#include <stdio.h>
#include <libintl.h>
#include <locale.h>
#include <m17n.h>
#include <errno.h>
#include <fcitx-config/xdg.h>
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

FcitxHotkey FCITX_M17N_UP[2] = {{NULL, FcitxKey_Up, 0}, {NULL, 0, 0}};
FcitxHotkey FCITX_M17N_DOWN[2] = {{NULL, FcitxKey_Down, 0}, {NULL, 0, 0}};

const char*
KeySymName (FcitxKeySym keyval);

INPUT_RETURN_VALUE FcitxM17NDoInput(void* arg, FcitxKeySym sym, unsigned state);
INPUT_RETURN_VALUE FcitxM17NDoInputInternal(IM* im, FcitxKeySym sym, unsigned int state);
static boolean LoadM17NConfig(FcitxM17NConfig* fs);
static void SaveM17NConfig(FcitxM17NConfig* fs);

char* mtextToUTF8(MText* mt)
{
    // TODO Verify that bufsize is "just enough" in worst scenerio.
    size_t bufsize = (mtext_len(mt) + 1) * UTF8_MAX_LENGTH;
    char* buf = (char*) fcitx_utils_malloc0(bufsize);

    MConverter* mconv = mconv_buffer_converter(Mcoding_utf_8, (unsigned char*) buf, bufsize);
    mconv_encode(mconv, mt);

    buf[mconv->nbytes] = '\0';
    mconv_free_converter(mconv);
    return buf;
}

// Don't use this for large indices or (worse) list iteration.
void *mplistSub(MPlist *head, size_t idx) {
    
    while (idx--) {
        if (!head)
            return NULL;
        head = mplist_next(head);
    }
    return mplist_value(head);
}

int GetPageSize(IM* im)
{
    int value = 0;
    MPlist* plist = minput_get_variable (im->owner->mim->language, im->owner->mim->name, msymbol ("candidates-group-size"));
    void* head = NULL, *state = NULL;
    if (plist) {
        head = mplistSub(plist, 0);
        state = mplistSub(head, 2);
    }
    if (state == NULL || state == Minherited) {
        MPlist* plist = minput_get_variable (Mt, Mnil, msymbol ("candidates-group-size"));
        if (!plist)
            return value;
        head = mplistSub(plist, 0);
        value = (int) ( (int64_t) (mplistSub(head, 3)));
    }
    return value;
}

inline static void setPreedit(FcitxInstance* inst, FcitxInputState* is, const char* s, int cursor_pos)
{
    FcitxInputContext* ic = FcitxInstanceGetCurrentIC(inst);
    FcitxMessages* m = FcitxInputStateGetClientPreedit(is);
    FcitxMessagesSetMessageCount(m, 0);
    FcitxMessagesAddMessageAtLast(m, MSG_INPUT, "%s", s);
    FcitxInputStateSetClientCursorPos(is, 
        fcitx_utf8_get_nth_char((char*)s, cursor_pos) - s);
    
    if (ic && (ic->contextCaps & CAPACITY_PREEDIT) == 0) {
        m = FcitxInputStateGetPreedit(is);
        FcitxMessagesSetMessageCount(m, 0);
        if (strlen(s) != 0) {
            FcitxMessagesAddMessageAtLast(m, MSG_INPUT, "%s", s);
            FcitxInputStateSetShowCursor(is, true);
            FcitxInputStateSetCursorPos(is, 
                fcitx_utf8_get_nth_char((char*)s, cursor_pos) - s);
        }
    }
}

INPUT_RETURN_VALUE FcitxM17NGetCandWord(void *arg, FcitxCandidateWord *cand) {
    
    IM* im = (IM*) arg;
    if (!im->owner->mic)
        return IRV_TO_PROCESS;
    int* idx = (int*) cand->priv;
    
    int lastIdx = im->owner->mic->candidate_index;
    do {
        if (*idx == im->owner->mic->candidate_index) {
            break;
        }
        if (*idx > im->owner->mic->candidate_index)
            FcitxM17NDoInputInternal(im, FcitxKey_Right, FcitxKeyState_None);
        else if (*idx < im->owner->mic->candidate_index)
            FcitxM17NDoInputInternal(im, FcitxKey_Left, FcitxKeyState_None);
        /* though useless, but take care if there is a bug cause freeze */
        if (lastIdx == im->owner->mic->candidate_index)
            break;
        lastIdx = im->owner->mic->candidate_index;
    } while(im->owner->mic->candidate_list && im->owner->mic->candidate_show);
    
    if (!im->owner->mic->candidate_list || !im->owner->mic->candidate_show || *idx != im->owner->mic->candidate_index)
        return IRV_TO_PROCESS;
    
    MPlist *head = im->owner->mic->candidate_list;
    
    int i = 0;
    while (1) {
        int len;
        if (mplist_key (head) == Mtext)
            len = mtext_len ((MText *) mplist_value (head));
        else
            len = mplist_length ((MPlist *) mplist_value (head));

        if (i + len > *idx)
            break;

        i += len;
        head = mplist_next (head);
    }
    
    int delta = *idx - i;
    
    FcitxKeySym sym = FcitxKey_1;
    if ((delta + 1) % 10 == 0)
        sym = FcitxKey_0;
    else
        sym += delta % 10;
    INPUT_RETURN_VALUE result = FcitxM17NDoInputInternal(im, sym, FcitxKeyState_None);;
    im->forward = false;
    return result;
}

INPUT_RETURN_VALUE FcitxM17NGetCandWords(void *arg)
{
    IM* im = (IM*) arg;
    if (!im->owner->mic)
        return IRV_TO_PROCESS;
    FcitxInstance* inst = im->owner->owner;
    FcitxInputState* is = FcitxInstanceGetInputState(inst);
    
    boolean toShow = false;
    
    if (im->owner->mic->preedit) {
        char* preedit = mtextToUTF8(im->owner->mic->preedit);
        toShow = toShow || (strlen(preedit) != 0);
        if (toShow) {
            FcitxLog(DEBUG, "preedit is %s", preedit);
            setPreedit(inst, is, preedit, im->owner->mic->cursor_pos);
        }
        free(preedit);
    }

    if (im->owner->mic->status) {
        char* mstatus = mtextToUTF8(im->owner->mic->status);
        toShow = toShow || (strlen(mstatus) != 0);
        if (strlen(mstatus) != 0) {
            FcitxLog(DEBUG, "IM status changed to %s", mstatus);
            // FcitxMessages* auxDown = FcitxInputStateGetAuxUpDown(is);
            // FcitxMessagesAddMessageAtLast(auxDown, MSG_TIPS, "%s ", mstatus);
        }
        free(mstatus);
    }

    FcitxCandidateWordList *cl = FcitxInputStateGetCandidateList(is);
    int value = GetPageSize(im);
    
    if (value)
        FcitxCandidateWordSetPageSize(cl, value);
    else
        FcitxCandidateWordSetPageSize(cl, 10);
    FcitxCandidateWordSetChoose(cl, DIGIT_STR_CHOOSE);
    FcitxCandidateWordReset(cl);

    FcitxCandidateWord cand;
    cand.owner = im;
    cand.callback = FcitxM17NGetCandWord;
    cand.priv = NULL;
    cand.strExtra = NULL;
    cand.wordType = MSG_OTHER;
    
    if (im->owner->mic->candidate_list && im->owner->mic->candidate_show) {
        MPlist *head = im->owner->mic->candidate_list;
        boolean flag = false;
        int index = 0;
        for (; head && mplist_key(head) != Mnil; head = mplist_next(head)) {
            MSymbol key = mplist_key(head);
            if (key == Mplist) {
                MPlist *head2 = mplist_value(head);
                for (; head2; head2 = mplist_next(head2)) {
                    MText *word = mplist_value(head2);
                    // Fcitx will do the free() for us.
                    cand.strWord = mtextToUTF8(word);
                    cand.priv = fcitx_utils_malloc0(sizeof(int));
                    int* candIdx = (int*) cand.priv;
                    *candIdx = index;
                    m17n_object_unref(word);
                    FcitxCandidateWordAppend(cl, &cand);
                    flag = true;
                    index ++;
                }
            } else if (key == Mtext) {
                char *words = mtextToUTF8((MText*) mplist_value(head));
                char *p, *q;
                for (p = words; *p; p = q) {
                    int unused;
                    q = fcitx_utf8_get_char(p, &unused);
                    cand.strWord = strndup(p, q-p);
                    cand.priv = fcitx_utils_malloc0(sizeof(int));
                    int* candIdx = (int*) cand.priv;
                    *candIdx = index;
                    FcitxCandidateWordAppend(cl, &cand);
                    flag = true;
                    index ++;
                }
                free(words);
            } else {
                FcitxLog(DEBUG, "Invalid MSymbol: %s", msymbol_name(key));
            }
        }
        toShow = toShow || flag;
    }
    
    if (im->forward)
        return IRV_DISPLAY_CANDWORDS | IRV_FLAG_FORWARD_KEY;
    else
        return IRV_DISPLAY_CANDWORDS;
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

    IM* im = (IM*) arg;
    if (!im->owner->mic)
        return IRV_TO_PROCESS;

    im->forward = false;
    FcitxInstance* inst = im->owner->owner;
    FcitxInputState* is = FcitxInstanceGetInputState(inst);
    
    if (FcitxCandidateWordGetListSize(FcitxInputStateGetCandidateList(is)) > 0
        && (
            FcitxHotkeyIsHotKeyDigit(sym, state)
            || FcitxHotkeyIsHotKey(sym, state, FCITX_M17N_UP)
            || FcitxHotkeyIsHotKey(sym, state, FCITX_M17N_DOWN)
            || FcitxHotkeyIsHotKey(sym, state, im->owner->config.hkPrevPage)
            || FcitxHotkeyIsHotKey(sym, state, im->owner->config.hkNextPage)
        ))
        return IRV_TO_PROCESS;
    
    if (FcitxCandidateWordGetListSize(FcitxInputStateGetCandidateList(is)) > 0
        && (FcitxHotkeyIsHotKey(sym, state, FCITX_RIGHT)
            || FcitxHotkeyIsHotKey(sym, state, FCITX_LEFT)))
        return IRV_DO_NOTHING;

    return FcitxM17NDoInputInternal(im, sym, state);
}

INPUT_RETURN_VALUE FcitxM17NDoInputInternal(IM* im, FcitxKeySym sym, unsigned int state)
{
    MSymbol msym = FcitxM17NKeySymToSymbol(sym, state);
    FcitxInstance* inst = im->owner->owner;
    FcitxInputContext* ic = FcitxInstanceGetCurrentIC(inst);

    if (msym == Mnil) {
        FcitxLog(DEBUG, "sym=%x, state=%x, not my dish", sym, state);
        return IRV_TO_PROCESS;
    }
    int thru = 0;
    if (!minput_filter(im->owner->mic, msym, NULL)) {
        MText* produced = mtext();
        // If input symbol was let through by m17n, let Fcitx handle it.
        // m17n may still produce some text to commit, though.
        thru = minput_lookup(im->owner->mic, msym, NULL, produced);
        if (mtext_len(produced) > 0) {
            char* buf = mtextToUTF8(produced);
            FcitxInstanceCommitString(inst, ic, buf);
            FcitxLog(DEBUG, "Commit: %s", buf);
            free(buf);
        }
        m17n_object_unref(produced);
    }
    
    if (thru)
        im->forward = true;

    return IRV_DISPLAY_CANDWORDS;
}

void FcitxM17NReset(void *arg)
{
    // FcitxLog(INFO, "Reset called, hahaha");
    IM* im = (IM*) arg;
    if (!im->owner->mic)
        return;
    minput_reset_ic(im->owner->mic);
}

boolean FcitxM17NInit(void *arg)
{
    // FcitxLog(INFO, "Init called, hahaha");
    IM* im = (IM*) arg;
    FcitxInstance* inst = im->owner->owner;
    boolean flag = true;
    FcitxInstanceSetContext(inst, CONTEXT_DISABLE_AUTOENG, &flag);
    FcitxInstanceSetContext(inst, CONTEXT_DISABLE_QUICKPHRASE, &flag);
    FcitxInstanceSetContext(inst, CONTEXT_ALTERNATIVE_PREVPAGE_KEY, im->owner->config.hkPrevPage);
    FcitxInstanceSetContext(inst, CONTEXT_ALTERNATIVE_NEXTPAGE_KEY, im->owner->config.hkNextPage);
    FcitxInstanceSetContext(inst, CONTEXT_IM_LANGUAGE, "us");
    
    if (im->owner->mim == NULL || im->owner->mim->language != im->mlang || im->owner->mim->name != im->mname) {
        if (im->owner->mic)
            minput_destroy_ic(im->owner->mic);
        
        if (im->owner->mim)
            minput_close_im(im->owner->mim);
        
        im->owner->mim = minput_open_im(im->mlang, im->mname, NULL);
        im->owner->mic = minput_create_ic(im->owner->mim, NULL);
    }
    
    return true;
}

void FcitxM17NReload(void *arg)
{
    IM* im = (IM*) arg;
    LoadM17NConfig(&im->owner->config);
}

void FcitxM17NSave(void *arg)
{
    // FcitxLog(INFO, "Save called, hahaha");
}

IM* makeIM(Addon* owner, MSymbol mlang, MSymbol mname)
{
    IM *im = (IM*) fcitx_utils_malloc0(sizeof(IM));
    im->owner = owner;
    im->mlang = mlang;
    im->mname = mname;
    // FcitxLog(INFO, "IM %s(%s) created, hahaha", name, lang);
    return im;
}

void delIM(IM* im)
{
    free(im);
}

void *FcitxM17NCreate(FcitxInstance* inst)
{
    bindtextdomain(TEXTDOMAIN, LOCALEDIR);
    

    Addon* addon = (Addon*) fcitx_utils_malloc0(sizeof(Addon));
    addon->owner = inst;

    if (!LoadM17NConfig(&addon->config))
    {
        free(addon);
        return NULL;
    }
    M17N_INIT();

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
    
    if (addon->mic)
        minput_destroy_ic(addon->mic);

    if (addon->mim)
        minput_close_im(addon->mim);
    
    free(addon);
    M17N_FINI();
}

CONFIG_DESC_DEFINE(GetM17NConfigDesc, "fcitx-m17n.desc")

static boolean LoadM17NConfig(FcitxM17NConfig* fs)
{
    FcitxConfigFileDesc *configDesc = GetM17NConfigDesc();
    if (!configDesc)
        return false;

    FILE *fp = FcitxXDGGetFileUserWithPrefix("conf", "fcitx-m17n.config", "rt", NULL);

    if (!fp)
    {
        if (errno == ENOENT)
            SaveM17NConfig(fs);
    }
    FcitxConfigFile *cfile = FcitxConfigParseConfigFileFp(fp, configDesc);

    FcitxM17NConfigConfigBind(fs, cfile, configDesc);
    FcitxConfigBindSync(&fs->gconfig);

    if (fp)
        fclose(fp);
    return true;
}

static void SaveM17NConfig(FcitxM17NConfig* fs)
{
    FcitxConfigFileDesc *configDesc = GetM17NConfigDesc();
    FILE *fp = FcitxXDGGetFileUserWithPrefix("conf", "fcitx-m17n.config", "wt", NULL);
    FcitxConfigSaveConfigFileFp(fp, &fs->gconfig, configDesc);
    if (fp)
        fclose(fp);
}