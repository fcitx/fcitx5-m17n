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
#include "keysymname.h"
#include "overrideparser.h"

#define CONF_FNAME "fcitx-m17n.config"
#define TEXTDOMAIN "fcitx-m17n"
#define _(x) dgettext(TEXTDOMAIN, x)

static const FcitxHotkey FCITX_M17N_UP[2] = {{NULL, FcitxKey_Up, 0}, {NULL, FcitxKey_P, FcitxKeyState_Ctrl}};
static const FcitxHotkey FCITX_M17N_DOWN[2] = {{NULL, FcitxKey_Down, 0}, {NULL, FcitxKey_N, FcitxKeyState_Ctrl}};

static MSymbol            KeySymToSymbol (FcitxKeySym sym, unsigned int state);

static void*              FcitxM17NCreate(FcitxInstance *instance);
static void               FcitxM17NDestroy(void *arg);
static void               FcitxM17NReset(void *arg);
static boolean            FcitxM17NInit(void *arg);
static INPUT_RETURN_VALUE FcitxM17NDoInput(void* arg, FcitxKeySym sym, unsigned state);
static INPUT_RETURN_VALUE FcitxM17NDoInputInternal(IM* im, FcitxKeySym sym, unsigned int state);
static INPUT_RETURN_VALUE FcitxM17NGetCandWord(void *arg, FcitxCandidateWord *cand);
static INPUT_RETURN_VALUE FcitxM17NGetCandWords(void *arg);
static void               FcitxM17NReload(void *arg);
static void               FcitxM17NSave(void *arg);
static IM*                FcitxM17NMakeIM(Addon* owner, MSymbol mlang, MSymbol mname);
static void               FcitxM17NDelIM(IM* im);

static boolean            FcitxM17NConfigLoad(FcitxM17NConfig* fs);
static void               FcitxM17NConfigSave(FcitxM17NConfig* fs);

static void               FcitxM17NCallback (MInputContext *context, MSymbol command);

static char*              MTextToUTF8(MText* mt);
static void*              MPListIndex(MPlist *head, size_t idx);

inline static void        SetPreedit(FcitxInstance* inst, FcitxInputState* is, const char* s, int cursor_pos);
static int                GetPageSize(MSymbol mlang, MSymbol mname);

FCITX_DEFINE_PLUGIN(fcitx_m17n, ime, FcitxIMClass) = {
    FcitxM17NCreate,
    FcitxM17NDestroy
};

char* MTextToUTF8(MText* mt)
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
void *MPListIndex(MPlist *head, size_t idx) {
    while (idx--) {
        head = mplist_next(head);
    }
    return mplist_value(head);
}

int GetPageSize(MSymbol mlang, MSymbol mname)
{
    MPlist* plist = minput_get_variable(
            mlang, mname, msymbol("candidates-group-size"));
    if (plist == NULL) {
        if (mlang == Mt && mname == Mnil) {
            // XXX magic number
            return 10;
        } else {
            // tail recursion
            return GetPageSize(Mt, Mnil);
        }
    }
    MPlist *varinfo = (MPlist*) mplist_value(plist);
    return (int) (intptr_t) MPListIndex(varinfo, 3);
}

inline static void SetPreedit(FcitxInstance* inst, FcitxInputState* is, const char* s, int cursor_pos)
{
    FcitxInputContext* ic = FcitxInstanceGetCurrentIC(inst);
    FcitxProfile* profile = FcitxInstanceGetProfile(inst);
    FcitxMessages* m = FcitxInputStateGetClientPreedit(is);
    FcitxMessagesSetMessageCount(m, 0);
    FcitxMessagesAddMessageAtLast(m, MSG_INPUT, "%s", s);
    FcitxInputStateSetClientCursorPos(is,
        fcitx_utf8_get_nth_char((char*)s, cursor_pos) - s);
    if (ic && ((ic->contextCaps & CAPACITY_PREEDIT) == 0 || !profile->bUsePreedit)) {
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

FcitxIRV FcitxM17NGetCandWord(void *arg, FcitxCandidateWord *cand)
{
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
        if (mplist_key (head) == Mtext) {
            len = mtext_len ((MText *) mplist_value (head));
        } else {
            len = mplist_length ((MPlist *) mplist_value (head));
        }

        if (i + len > *idx) {
            break;
        }

        i += len;
        head = mplist_next (head);
    }

    int delta = *idx - i;

    FcitxKeySym sym = FcitxKey_1;
    if ((delta + 1) % 10 == 0)
        sym = FcitxKey_0;
    else
        sym += delta % 10;
    FcitxIRV result = FcitxM17NDoInputInternal(im, sym, FcitxKeyState_None);;
    im->forward = false;
    return result;
}

FcitxIRV FcitxM17NGetCandWords(void *arg)
{
    IM* im = (IM*) arg;
    if (!im->owner->mic)
        return IRV_TO_PROCESS;
    FcitxInstance* inst = im->owner->owner;
    FcitxInputState* is = FcitxInstanceGetInputState(inst);

    boolean toShow = false;
    FcitxIRV ret = IRV_DO_NOTHING;

    if (im->owner->mic->preedit) {
        char* preedit = MTextToUTF8(im->owner->mic->preedit);
        toShow = toShow || (strlen(preedit) != 0);
        if (strlen(preedit) > 0) {
            FcitxLog(DEBUG, "preedit is %s", preedit);
            SetPreedit(inst, is, preedit, im->owner->mic->cursor_pos);
        }
        free(preedit);
    }

    if (im->owner->mic->status) {
        char* mstatus = MTextToUTF8(im->owner->mic->status);
        // toShow = toShow || (strlen(mstatus) != 0);
        if (strlen(mstatus) != 0) {
            FcitxLog(DEBUG, "IM status changed to %s", mstatus);
            // FcitxMessages* auxDown = FcitxInputStateGetAuxUpDown(is);
            // FcitxMessagesAddMessageAtLast(auxDown, MSG_TIPS, "%s ", mstatus);
        }
        free(mstatus);
    }

    FcitxCandidateWordList *cl = FcitxInputStateGetCandidateList(is);
    FcitxCandidateWordSetPageSize(cl, im->pageSize);
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
        boolean hasCand = false;
        int index = 0;
        for (; head && mplist_key(head) != Mnil; head = mplist_next(head)) {
            MSymbol key = mplist_key(head);
            if (key == Mplist) {
                MPlist *head2 = mplist_value(head);
                for (; head2 && mplist_key(head2) != Mnil; head2 = mplist_next(head2)) {
                    MText *word = mplist_value(head2);
                    // Fcitx will do the free() for us.
                    cand.strWord = MTextToUTF8(word);
                    cand.priv = fcitx_utils_malloc0(sizeof(int));
                    int* candIdx = (int*) cand.priv;
                    *candIdx = index;
                    FcitxCandidateWordAppend(cl, &cand);
                    hasCand = true;
                    index ++;
                }
            } else if (key == Mtext) {
                char *words = MTextToUTF8((MText*) mplist_value(head));
                char *p, *q;
                for (p = words; *p; p = q) {
                    uint32_t unused;
                    q = fcitx_utf8_get_char(p, &unused);
                    cand.strWord = strndup(p, q-p);
                    cand.priv = fcitx_utils_malloc0(sizeof(int));
                    int* candIdx = (int*) cand.priv;
                    *candIdx = index;
                    FcitxCandidateWordAppend(cl, &cand);
                    hasCand = true;
                    index ++;
                }
                free(words);
            } else {
                FcitxLog(DEBUG, "Invalid MSymbol: %s", msymbol_name(key));
            }
        }
        toShow = toShow || hasCand;
    }
    FcitxUIUpdateInputWindow(inst);

    if (im->forward) {
        ret = IRV_TO_PROCESS;
    }
    return ret;
}

MSymbol KeySymToSymbol (FcitxKeySym sym, unsigned int state)
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
    char temp[2] = " ";

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
    if (mask & FcitxKeyState_Super) {
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

FcitxIRV FcitxM17NDoInput(void* arg, FcitxKeySym sym, unsigned state)
{
    // FcitxLog(INFO, "DoInput got sym=%x, state=%x, hahaha", sym, state);

    IM* im = (IM*) arg;
    if (!im->owner->mic)
        return IRV_TO_PROCESS;

    im->forward = false;
    FcitxInstance* inst = im->owner->owner;
    FcitxInputState* is = FcitxInstanceGetInputState(inst);
    int candSize = FcitxCandidateWordGetListSize(FcitxInputStateGetCandidateList(is));

    if (candSize > 0) {
        if (FcitxHotkeyIsHotKeyDigit(sym, state)
            || FcitxHotkeyIsHotKey(sym, state, FCITX_M17N_UP)
            || FcitxHotkeyIsHotKey(sym, state, FCITX_M17N_DOWN)
            || FcitxHotkeyIsHotKey(sym, state, im->owner->config.hkPrevPage)
            || FcitxHotkeyIsHotKey(sym, state, im->owner->config.hkNextPage)) {
            return IRV_TO_PROCESS;
        }

        if (FcitxHotkeyIsHotKey(sym, state, FCITX_RIGHT)
            || FcitxHotkeyIsHotKey(sym, state, FCITX_LEFT)) {
            return IRV_DO_NOTHING;
        }
    }

    return FcitxM17NDoInputInternal(im, sym, state);
}

FcitxIRV FcitxM17NDoInputInternal(IM* im, FcitxKeySym sym, unsigned state)
{
    FcitxInstance* inst = im->owner->owner;
    FcitxInputState* is = FcitxInstanceGetInputState(inst);
    FcitxInputContext* ic = FcitxInstanceGetCurrentIC(inst);

    MSymbol msym = KeySymToSymbol(sym, state);

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
            char* buf = MTextToUTF8(produced);
            FcitxInstanceCommitString(inst, ic, buf);
            FcitxLog(DEBUG, "Commit: %s", buf);
            free(buf);
        }
        m17n_object_unref(produced);
    }

    im->forward = thru;

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
    FcitxInstanceSetContext(inst, CONTEXT_IM_KEYBOARD_LAYOUT, "us");

    if (im->owner->mim == NULL || im->owner->mim->language != im->mlang || im->owner->mim->name != im->mname) {
        if (im->owner->mic)
            minput_destroy_ic(im->owner->mic);

        if (im->owner->mim)
            minput_close_im(im->owner->mim);

        im->owner->mim = minput_open_im(im->mlang, im->mname, NULL);
        mplist_put (im->owner->mim->driver.callback_list, Minput_get_surrounding_text, FcitxM17NCallback);
        mplist_put (im->owner->mim->driver.callback_list, Minput_delete_surrounding_text, FcitxM17NCallback);
        im->owner->mic = minput_create_ic(im->owner->mim, im);
        if (!im->pageSize)
            im->pageSize = GetPageSize(im->mlang, im->mname);
    }

    return true;
}

void FcitxM17NReload(void *arg)
{
    IM* im = (IM*) arg;
    FcitxM17NConfigLoad(&im->owner->config);
}

void FcitxM17NSave(void *arg)
{
    // FcitxLog(INFO, "Save called, hahaha");
}

IM* FcitxM17NMakeIM(Addon* owner, MSymbol mlang, MSymbol mname)
{
    IM *im = (IM*) fcitx_utils_malloc0(sizeof(IM));
    im->owner = owner;
    im->mlang = mlang;
    im->mname = mname;
    // FcitxLog(INFO, "IM %s(%s) created, hahaha", name, lang);
    return im;
}

void FcitxM17NDelIM(IM* im)
{
    free(im);
}

void *FcitxM17NCreate(FcitxInstance* inst)
{
    bindtextdomain(TEXTDOMAIN, LOCALEDIR);

    Addon* addon = (Addon*) fcitx_utils_malloc0(sizeof(Addon));
    addon->owner = inst;

    if (!FcitxM17NConfigLoad(&addon->config)) {
        free(addon);
        return NULL;
    }
    M17N_INIT();

    MPlist *mimlist = minput_list(Mnil);
    addon->nim = mplist_length(mimlist);
    // This wastes some space if some of the m17n IM's are not "sane",
    // but it also makes the the code a little simpler.
    addon->ims = (IM**) fcitx_utils_malloc0(sizeof(IM*) * addon->nim);

    OverrideItemList* list = NULL;
    FILE* fp = FcitxXDGGetFileWithPrefix("m17n", "default", "r", NULL);
    if (fp) {
        list = ParseDefaultSettings(fp);
        fclose(fp);
    }

    char* curlang = fcitx_utils_get_current_langcode();
    int i;
    for (i = 0; i < addon->nim; i++, mimlist = mplist_next(mimlist)) {
        // See m17n documentation of minput_list() in input.c.
        MPlist *info;
        info = (MPlist*) mplist_value(mimlist);
        MSymbol mlang = (MSymbol) MPListIndex(info, 0);
        MSymbol mname = (MSymbol) MPListIndex(info, 1);
        MSymbol msane = (MSymbol) MPListIndex(info, 2);

        char *lang = msymbol_name(mlang);
        char *name = msymbol_name(mname);

        OverrideItem* item = NULL;
        if (list)
            item = MatchDefaultSettings(list, lang, name);

        if (item && item->priority < 0 && !addon->config.enableDeprecated)
            continue;

        if (msane != Mt) {
            // Not "sane"
            // FcitxLog(WARNING, "Insane IM [%s: %s]", lang, name);
            continue;
        }

        MPlist* l = minput_get_variable(mlang, mname, msymbol("candidates-charset"));
        if (l) {
            /* XXX Non-utf8 encodings are ditched. */
            if (((MSymbol) MPListIndex(mplist_value(l), 3)) != Mcoding_utf_8) {
                continue;
            }
        }

        if (!(addon->ims[i] = FcitxM17NMakeIM(addon, mlang, mname))) {
            continue;
        }
        FcitxLog(DEBUG, "Created IM [%s: %s]", lang, name);

        char *uniqueName, *fxName, *iconName;
        asprintf(&uniqueName, "m17n_%s_%s", lang, name);
        asprintf(&fxName, _("%s (M17N)"), (item && item->i18nName) ? _(item->i18nName) : name);

        info = minput_get_title_icon(mlang, mname);
        // head of info is a MText
        MText *iconPath = (MText*) MPListIndex(info, 1);

        if (iconPath) {
            // This disregards the fact that the path to m17n icon files
            // might have utf8-incompatible path strings. However, since the
            // path almost always consists of ascii characters (typically
            // /usr/share/m17n/icons/... on Linux systems, this is a
            // reasonable assumption.
            iconName = MTextToUTF8(iconPath);
            FcitxLog(DEBUG, "Mim icon is %s", iconName);
        } else {
            iconName = uniqueName;
        }

        FcitxIMIFace iface;
        memset(&iface, 0, sizeof(FcitxIMIFace));
        iface.Init = FcitxM17NInit;
        iface.DoInput = FcitxM17NDoInput;
        iface.ResetIM = FcitxM17NReset;
        iface.Save = FcitxM17NSave;
        iface.ReloadConfig = FcitxM17NReload;
        iface.GetCandWords = FcitxM17NGetCandWords;

        int priority = 100;
        if (item && strncmp(curlang, lang, 2) == 0 && item->priority > 0)
            priority = item->priority;

        FcitxInstanceRegisterIMv2(
            inst, addon->ims[i],
            uniqueName, fxName, iconName,
            iface,
            priority, strcmp(lang, "t") == 0 ? "*" : lang
        );
        if (iconName != uniqueName) {
            free(iconName);
        }
        free(uniqueName);
        free(fxName);
    }

    fcitx_utils_free(curlang);

    if (list)
        utarray_free(list);

    return addon;
}

void FcitxM17NDestroy(void *arg)
{
    Addon* addon = (Addon*) arg;
    int i;
    for (i = 0; i < addon->nim; i++) {
        if (addon->ims[i]) {
            FcitxM17NDelIM(addon->ims[i]);
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

static boolean FcitxM17NConfigLoad(FcitxM17NConfig* fs)
{
    FcitxConfigFileDesc *configDesc = GetM17NConfigDesc();
    if (!configDesc) {
        return false;
    }

    FILE *fp = FcitxXDGGetFileUserWithPrefix("conf", CONF_FNAME, "r", NULL);

    if (!fp) {
        if (errno == ENOENT)
            FcitxM17NConfigSave(fs);
    }
    FcitxConfigFile *cfile = FcitxConfigParseConfigFileFp(fp, configDesc);

    FcitxM17NConfigConfigBind(fs, cfile, configDesc);
    FcitxConfigBindSync(&fs->gconfig);

    if (fp) {
        fclose(fp);
    }
    return true;
}

static void FcitxM17NConfigSave(FcitxM17NConfig* fs)
{
    FcitxConfigFileDesc *configDesc = GetM17NConfigDesc();
    FILE *fp = FcitxXDGGetFileUserWithPrefix("conf", CONF_FNAME, "w", NULL);
    FcitxConfigSaveConfigFileFp(fp, &fs->gconfig, configDesc);
    if (fp) {
        fclose(fp);
    }
}


static void
FcitxM17NCallback (MInputContext *context,
                   MSymbol command)
{
    IM* im = (IM*) context->arg;

    if (!im)
        return;

    FcitxInputContext* ic = FcitxInstanceGetCurrentIC(im->owner->owner);

    if (command == Minput_get_surrounding_text && ic && (ic->contextCaps & CAPACITY_SURROUNDING_TEXT)) {
        char* text = NULL;
        unsigned int cursor_pos;

        if (!FcitxInstanceGetSurroundingText (im->owner->owner, ic, &text, &cursor_pos, NULL))
            return;
        if (!text)
            return;
        size_t nchars = fcitx_utf8_strlen(text);
        size_t nbytes = strlen(text);
        MText* mt = mconv_decode_buffer (Mcoding_utf_8, (const unsigned char*) text, nbytes), *surround = NULL;
        free(text);
        if (!mt)
            return;

        long len = (long) mplist_value (context->plist), pos;
        if (len < 0) {
            pos = cursor_pos + len;
            if (pos < 0)
                pos = 0;
            surround = mtext_duplicate (mt, pos, cursor_pos);
        }
        else if (len > 0) {
            pos = cursor_pos + len;
            if (pos > nchars)
                pos = nchars;
            surround = mtext_duplicate (mt, cursor_pos, pos);
        }
        else {
            surround = mtext ();
        }
        m17n_object_unref (mt);
        if (surround) {
            mplist_set (context->plist, Mtext, surround);
            m17n_object_unref (surround);
        }
    }
    else if (command == Minput_delete_surrounding_text && ic && (ic->contextCaps & CAPACITY_SURROUNDING_TEXT)) {
        int len;

        len = (long) mplist_value (context->plist);
        if (len < 0)
            FcitxInstanceDeleteSurroundingText (im->owner->owner, ic, len, -len);
        else if (len > 0)
            FcitxInstanceDeleteSurroundingText (im->owner->owner, ic, 0, -len);
    }
}
