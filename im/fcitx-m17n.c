// See COPYING in distribution for license.
#include <string.h>
#include <stdlib.h>
#include <stdio.h>
#include <libintl.h>
#include <locale.h>
#include <m17n.h>
#include <fcitx-utils/log.h>
#include <fcitx/keys.h>
#include <fcitx/instance.h>
#include <fcitx/context.h>

#include "fcitx-m17n.h"

#define _(x) gettext(x)

FCITX_EXPORT_API
FcitxIMClass ime = {
    FcitxM17NCreate,
    FcitxM17NDestroy
};

FCITX_EXPORT_API
int ABI_VERSION = 5;

char* mtextToUTF8(MText* mt) {
    // TODO Verify that bufsize is "just enough" in worst scenerio.
    size_t bufsize = mtext_len(mt) * 6 + 6;
    char* buf = (char*) malloc(bufsize);

    MConverter* mconv = mconv_buffer_converter(Mcoding_utf_8, (unsigned char*) buf, bufsize);
    mconv_encode(mconv, mt);
    mconv_free_converter(mconv);

    buf[mconv->nbytes] = '\0';
    return buf;
}

inline static void setPreedit(FcitxInputState* is, const char* s) {
    FcitxMessages* m = FcitxInputStateGetClientPreedit(is);
    FcitxMessagesSetMessageCount(m, 1);
    FcitxMessagesAddMessageAtLast(m, MSG_INPUT, "%s", s);
}

// Never called since FcitxM17NDoInput never returns IRV_SHOW_CANDWORDS.
INPUT_RETURN_VALUE FcitxM17NGetCandWords(void *arg) {
    return IRV_CLEAN;
}

typedef struct {
    FcitxHotkey* key;
    char* value;
} hotkeyMap;

static hotkeyMap hotkeyMaps[] = {
    FCITX_BACKSPACE, "BackSpace",
    FCITX_ENTER, "Linefeed",
    NULL, NULL
};

INPUT_RETURN_VALUE FcitxM17NDoInput(void* arg, FcitxKeySym sym, unsigned state) {
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
    MSymbol msym;
    if (FcitxHotkeyIsHotKeySimple(sym, state)) {
    	char ssym[2];
    	ssym[0] = sym & 0x7f;
    	ssym[1] = '\0';
    	msym = msymbol(ssym);
    } else {
        hotkeyMap* p;
        for (p = hotkeyMaps; p->key; p++) {
            if (FcitxHotkeyIsHotKey(sym, state, p->key)) {
                msym = msymbol(p->value);
                break;
            }
        }
        if (!p->key) {
            FcitxLog(INFO, "sym=%x, state=%x, not my dish", sym, state);
            return IRV_TO_PROCESS;
        }
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
    FcitxInputStateSetClientCursorPos(is, im->mic->cursor_pos);

    FcitxInstanceUpdatePreedit(inst, ic);
    // FcitxInstanceUpdateClientSideUI(inst, ic);
    free(preedit);

    char* mstatus = mtextToUTF8(im->mic->status);
    FcitxLog(INFO, "mic status is %s", mstatus);
    free(mstatus);

    return thru ? IRV_TO_PROCESS : IRV_DO_NOTHING;
}

void FcitxM17NReset(void *arg) {
    // FcitxLog(INFO, "Reset called, hahaha");
    IM* im = (IM*) arg;
    minput_reset_ic(im->mic);
}

boolean FcitxM17NInit(void *arg) {
    // FcitxLog(INFO, "Init called, hahaha");
    IM* im = (IM*) arg;
    FcitxInstance* inst = im->owner->owner;
    boolean flag = true;
    FcitxInstanceSetContext(inst, CONTEXT_DISABLE_AUTOENG, &flag);
    FcitxInstanceSetContext(inst, CONTEXT_DISABLE_QUICKPHRASE, &flag);
    return true;
}

void FcitxM17NReload(void *arg) {
    // FcitxLog(INFO, "Reload called, hahaha");
}

void FcitxM17NSave(void *arg) {
    // FcitxLog(INFO, "Save called, hahaha");
}

IM* makeIM(Addon* owner, const char* lang, const char* name) {
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
    IM *im = (IM*) malloc(sizeof(IM));
    im->owner = owner;
    im->mim = mim;
    im->mic = mic;
    // FcitxLog(INFO, "IM %s(%s) created, hahaha", name, lang);
    return im;
}

void delIM(IM* im) {
    minput_destroy_ic(im->mic);
    minput_close_im(im->mim);
    free(im);
}

typedef struct {
    const char* lang;
    const char* name;
} m17nIM;

static m17nIM m17nIMs[] = {
    "t", "latn-post",
    "el", "kbd",
    "eo", "plena",
    "ja", "anthy",
    NULL, NULL
};

void *FcitxM17NCreate(FcitxInstance* inst) {
    bindtextdomain("fcitx-m17n", LOCALEDIR);
    setlocale(LC_ALL, "");
    M17N_INIT();

    Addon* addon = (Addon*) fcitx_utils_malloc0(sizeof(Addon));
    addon->owner = inst;

    // TODO Built-in list of IMs only.
    addon->nim = 0;
    m17nIM* p;
    for (p = m17nIMs; p->lang; p++) {
        addon->nim++;
    }

    addon->ims = (IM**) malloc(sizeof(IM*) * addon->nim);

    int i = 0;
    for (m17nIM* p = m17nIMs; p->lang; p++, i++) {
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
    		5, "en_US"
    	);
    }

    return addon;
}

void FcitxM17NDestroy(void *arg) {
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

