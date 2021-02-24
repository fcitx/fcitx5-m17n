/*
 * SPDX-FileCopyrightText: 2017~2017 CSSlayer <wengxt@gmail.com>
 *
 * SPDX-License-Identifier: LGPL-2.1-or-later
 *
 */
#include "engine.h"
#include "keysymname.h"
#include "overrideparser.h"
#include <fcitx-utils/i18n.h>
#include <fcitx-utils/log.h>
#include <fcitx-utils/standardpath.h>
#include <fcitx-utils/utf8.h>
#include <fcitx/inputcontext.h>
#include <fcitx/inputcontextmanager.h>
#include <fcitx/inputpanel.h>
#include <fcitx/instance.h>
#include <fcntl.h>
#include <fmt/format.h>
#include <m17n.h>
#include <string>

FCITX_DEFINE_LOG_CATEGORY(M17N, "m17n")

#define FCITX_M17N_DEBUG() FCITX_LOGC(M17N, Debug)
#define FCITX_M17N_WARN() FCITX_LOGC(M17N, Warn)

namespace fcitx {

namespace {

std::string MTextToUTF8(MText *mt) {
    // TODO Verify that bufsize is "just enough" in worst scenerio.
    size_t bufsize = (mtext_len(mt) + 1) * FCITX_UTF8_MAX_LENGTH;
    std::vector<char> buf;
    buf.resize(bufsize);
    FCITX_M17N_DEBUG() << "MText buf size: " << bufsize;

    MConverter *mconv = mconv_buffer_converter(
        Mcoding_utf_8, reinterpret_cast<unsigned char *>(buf.data()), bufsize);
    mconv_encode(mconv, mt);

    buf[mconv->nbytes] = '\0';
    FCITX_M17N_DEBUG() << "MText bytes: " << mconv->nbytes;
    mconv_free_converter(mconv);
    return buf.data();
}

// Don't use this for large indices or (worse) list iteration.
void *MPListIndex(MPlist *head, size_t idx) {
    while (idx--) {
        head = mplist_next(head);
    }
    return mplist_value(head);
}

MSymbol KeySymToSymbol(Key key) {
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
    KeyStates mask;

    if (key.sym() >= FcitxKey_Shift_L && key.sym() <= FcitxKey_Hyper_R) {
        return Mnil;
    }

    std::string base;
    char temp[2] = " ";

    if (key.sym() >= FcitxKey_space && key.sym() <= FcitxKey_asciitilde) {
        KeySym c = key.sym();

        if (key.sym() == FcitxKey_space && key.states().test(KeyState::Shift)) {
            mask |= KeyState::Shift;
        }

        if (key.states().test(KeyState::Ctrl)) {
            if (c >= FcitxKey_a && c <= FcitxKey_z) {
                c = static_cast<KeySym>(c + FcitxKey_A - FcitxKey_a);
            }
            mask |= KeyState::Ctrl;
        }

        temp[0] = c & 0xff;
        base = temp;
    } else {
        mask |= key.states() & (KeyState::Ctrl_Shift);
        base = KeySymName(key.sym());
        if (base.empty()) {
            return Mnil;
        }
    }

    mask |=
        key.states() & KeyStates{KeyState::Mod1, KeyState::Mod5, KeyState::Meta,
                                 KeyState::Super, KeyState::Hyper};

    // we have 7 possible below, then 20 is long enough (7 x 2 = 14 < 20)
    char prefix[20] = "";

    // and we use reverse order here comparing with other implementation since
    // strcat is append.
    // I don't know if it matters, but it's just to make sure it works.
    if (mask & KeyState::Shift) {
        strcat(prefix, "S-");
    }
    if (mask & KeyState::Ctrl) {
        strcat(prefix, "C-");
    }
    if (mask & KeyState::Meta) {
        strcat(prefix, "M-");
    }
    if (mask & KeyState::Alt) {
        strcat(prefix, "A-");
    }
    // This is mysterious. - xiaq
    if (mask & KeyState::Mod5) {
        strcat(prefix, "G-");
    }
    if (mask & KeyState::Super) {
        strcat(prefix, "s-");
    }
    if (mask & KeyState::Hyper) {
        strcat(prefix, "H-");
    }

    std::string keystr = stringutils::concat(static_cast<char *>(prefix), base);
    FCITX_M17N_DEBUG() << "M17n key str: " << keystr << " " << key;
    mkeysym = msymbol(keystr.data());

    return mkeysym;
}

int GetPageSize(MSymbol mlang, MSymbol mname) {
    MPlist *plist =
        minput_get_variable(mlang, mname, msymbol("candidates-group-size"));
    if (plist == NULL) {
        if (mlang == Mt && mname == Mnil) {
            // XXX magic number
            return 10;
        } else {
            // tail recursion
            return GetPageSize(Mt, Mnil);
        }
    }
    MPlist *varinfo = (MPlist *)mplist_value(plist);
    return reinterpret_cast<intptr_t>(MPListIndex(varinfo, 3));
}

inline static void SetPreedit(InputContext *ic, const std::string &s,
                              int cursor_pos) {
    Text preedit;
    preedit.append(s, TextFormatFlag::Underline);
    if (cursor_pos >= 0 && utf8::length(s) >= static_cast<size_t>(cursor_pos)) {
        preedit.setCursor(utf8::ncharByteLength(s.begin(), cursor_pos));
    }

    if (ic->capabilityFlags().test(CapabilityFlag::Preedit)) {
        ic->inputPanel().setClientPreedit(preedit);
    } else {
        ic->inputPanel().setPreedit(preedit);
    }
}

class M17NCandidateWord : public CandidateWord {
public:
    M17NCandidateWord(M17NEngine *engine, std::string text, int index)
        : CandidateWord(Text(std::move(text))), engine_(engine), index_(index) {
    }

    void select(InputContext *inputContext) const override {
        auto state = inputContext->propertyFor(engine_->factory());
        state->select(index_);
    }

private:
    M17NEngine *engine_;
    int index_;
};

class M17NCandidateList : public CommonCandidateList {
public:
    M17NCandidateList(M17NEngine *engine, InputContext *ic)
        : engine_(engine), ic_(ic) {
        auto state = ic_->propertyFor(engine_->factory());
        auto pageSize = GetPageSize(state->mim_->language, state->mim_->name);

        const static KeyList selectionKeys{
            Key(FcitxKey_1), Key(FcitxKey_2), Key(FcitxKey_3), Key(FcitxKey_4),
            Key(FcitxKey_5), Key(FcitxKey_6), Key(FcitxKey_7), Key(FcitxKey_8),
            Key(FcitxKey_9), Key(FcitxKey_0)};
        setPageSize(pageSize);
        setSelectionKey(selectionKeys);
        MPlist *head = state->mic_->candidate_list;
        int index = 0;
        for (; head && mplist_key(head) != Mnil; head = mplist_next(head)) {
            MSymbol key = mplist_key(head);
            if (key == Mplist) {
                MPlist *head2 = static_cast<MPlist *>(mplist_value(head));
                for (; head2 && mplist_key(head2) != Mnil;
                     head2 = mplist_next(head2)) {
                    MText *word = static_cast<MText *>(mplist_value(head2));
                    // Fcitx will do the free() for us.
                    auto str = MTextToUTF8(word);
                    append<M17NCandidateWord>(engine_, str, index);
                    index++;
                }
            } else if (key == Mtext) {
                auto words =
                    MTextToUTF8(static_cast<MText *>(mplist_value(head)));
                auto range = utf8::MakeUTF8CharRange(words);
                for (auto p = std::begin(range), q = std::end(range); p != q;
                     ++p) {
                    auto charRange = p.charRange();
                    std::string str(charRange.first, charRange.second);
                    append<M17NCandidateWord>(engine_, str, index);
                    index++;
                }
            } else {
                FCITX_M17N_DEBUG() << "Invalid MSymbol: " << msymbol_name(key);
            }
        }
        if (state->mic_->candidate_index >= 0 &&
            state->mic_->candidate_index < totalSize()) {
            setGlobalCursorIndex(state->mic_->candidate_index);
            setPage(state->mic_->candidate_index / pageSize);
        }
    }

    void prev() override {
        auto state = ic_->propertyFor(engine_->factory());
        state->keyEvent(Key(FcitxKey_Up));
    }

    void next() override {
        auto state = ic_->propertyFor(engine_->factory());
        state->keyEvent(Key(FcitxKey_Down));
    }

    bool usedNextBefore() const override { return true; }

    void prevCandidate() override {
        auto state = ic_->propertyFor(engine_->factory());
        state->keyEvent(Key(FcitxKey_Left));
    }

    void nextCandidate() override {
        auto state = ic_->propertyFor(engine_->factory());
        state->keyEvent(Key(FcitxKey_Right));
    }

private:
    M17NEngine *engine_;
    InputContext *ic_;
};

} // namespace

M17NEngine::M17NEngine(Instance *instance)
    : instance_(instance),
      factory_([this](InputContext &ic) { return new M17NState(this, &ic); }) {
    reloadConfig();
    M17N_INIT();

    auto file = StandardPath::global().open(StandardPath::Type::PkgData,
                                            "m17n/default", O_RDONLY);
    FILE *fp = fdopen(file.fd(), "r");
    if (fp) {
        file.release();
        list_ = ParseDefaultSettings(fp);
        fclose(fp);
    }

    instance_->inputContextManager().registerProperty("m17nState", &factory_);
}

std::vector<InputMethodEntry> M17NEngine::listInputMethods() {
    std::vector<InputMethodEntry> entries;
    MPlist *mimlist = minput_list(Mnil);
    auto imLength = mplist_length(mimlist);
    for (int i = 0; i < imLength; i++, mimlist = mplist_next(mimlist)) {
        // See m17n documentation of minput_list() in input.c.
        MPlist *info;
        info = (MPlist *)mplist_value(mimlist);
        MSymbol mlang = (MSymbol)MPListIndex(info, 0);
        MSymbol mname = (MSymbol)MPListIndex(info, 1);
        MSymbol msane = (MSymbol)MPListIndex(info, 2);

        char *lang = msymbol_name(mlang);
        char *name = msymbol_name(mname);

        const OverrideItem *item = nullptr;
        if (list_.size()) {
            item = MatchDefaultSettings(list_, lang, name);
        }

        if (item && item->priority < 0 && !*config_.enableDeprecated)
            continue;

        if (msane != Mt) {
            // Not "sane"
            FCITX_M17N_WARN() << "Insane IM [" << lang << ": " << name << "]";
            continue;
        }

        MPlist *l =
            minput_get_variable(mlang, mname, msymbol("candidates-charset"));
        if (l) {
            /* XXX Non-utf8 encodings are ditched. */
            if (((MSymbol)MPListIndex(static_cast<MPlist *>(mplist_value(l)),
                                      3)) != Mcoding_utf_8) {
                continue;
            }
        }

        FCITX_M17N_DEBUG() << "Created IM [" << lang << ": " << name << "]";

        std::string iconName;
        auto uniqueName = stringutils::concat("m17n_", lang, "_", name);
        auto fxName = fmt::format(
            _("{0} (M17N)"),
            (item && item->i18nName.size()) ? _(item->i18nName) : name);

        info = minput_get_title_icon(mlang, mname);
        // head of info is a MText
        MText *iconPath = static_cast<MText *>(MPListIndex(info, 1));

        if (iconPath) {
            // This disregards the fact that the path to m17n icon files
            // might have utf8-incompatible path strings. However, since the
            // path almost always consists of ascii characters (typically
            // /usr/share/m17n/icons/... on Linux systems, this is a
            // reasonable assumption.
            iconName = MTextToUTF8(iconPath);
            FCITX_M17N_DEBUG() << "Mim icon is " << iconName;
        } else {
            iconName = uniqueName;
        }
        m17n_object_unref(info);

        InputMethodEntry entry(uniqueName, fxName,
                               (strcmp(lang, "t") == 0 ? "*" : lang), "m17n");
        entry.setConfigurable(true).setIcon(iconName);
        entry.setUserData(std::make_unique<M17NData>(mlang, mname));
        entries.emplace_back(std::move(entry));
    }
    m17n_object_unref(mimlist);
    return entries;
}

void M17NEngine::activate(const InputMethodEntry &, InputContextEvent &) {}

void M17NEngine::deactivate(const InputMethodEntry &,
                            InputContextEvent &event) {
    auto inputContext = event.inputContext();
    auto state = inputContext->propertyFor(&factory_);
    if (event.type() == EventType::InputContextSwitchInputMethod) {
        state->commitPreedit();
    }
    state->reset();
}

void M17NEngine::keyEvent(const InputMethodEntry &entry, KeyEvent &keyEvent) {
    if (keyEvent.isRelease()) {
        return;
    }

    auto ic = keyEvent.inputContext();
    auto state = ic->propertyFor(&factory_);

    state->keyEvent(entry, keyEvent);
}

void M17NEngine::reset(const InputMethodEntry &, InputContextEvent &event) {
    auto ic = event.inputContext();
    auto state = ic->propertyFor(&factory_);
    state->reset();
}

void M17NEngine::reloadConfig() { readAsIni(config_, "conf/m17n.conf"); }

void M17NState::callback(MInputContext *context, MSymbol command) {
    M17NState *state = static_cast<M17NState *>(context->arg);
    state->command(context, command);
}

void M17NState::command(MInputContext *context, MSymbol command) {
    if (command == Minput_get_surrounding_text &&
        ic_->capabilityFlags().test(CapabilityFlag::SurroundingText) &&
        ic_->surroundingText().isValid()) {
        const auto &text = ic_->surroundingText().text();
        size_t nchars = utf8::length(text);
        size_t nbytes = text.size();
        MText *mt = mconv_decode_buffer(
                  Mcoding_utf_8,
                  reinterpret_cast<const unsigned char *>(text.data()), nbytes),
              *surround = nullptr;
        if (!mt)
            return;

        long len = (long)mplist_value(context->plist), pos;
        auto cursor = ic_->surroundingText().cursor();
        if (len < 0) {
            pos = cursor + len;
            if (pos < 0)
                pos = 0;
            surround = mtext_duplicate(mt, pos, cursor);
        } else if (len > 0) {
            pos = cursor + len;
            if (pos > static_cast<long>(nchars))
                pos = nchars;
            surround = mtext_duplicate(mt, cursor, pos);
        } else {
            surround = mtext();
        }
        m17n_object_unref(mt);
        if (surround) {
            mplist_set(context->plist, Mtext, surround);
            m17n_object_unref(surround);
        }
    } else if (command == Minput_delete_surrounding_text &&
               ic_->capabilityFlags().test(CapabilityFlag::SurroundingText)) {
        int len = reinterpret_cast<long>(mplist_value(context->plist));
        if (len < 0) {
            ic_->deleteSurroundingText(len, -len);
        } else if (len > 0) {
            ic_->deleteSurroundingText(0, len);
        }
    }
}

void M17NState::keyEvent(const InputMethodEntry &entry, KeyEvent &keyEvent) {
    auto data = static_cast<const M17NData *>(entry.userData());
    if (!mim_ || data->language() != mim_->language ||
        data->name() != mim_->name) {
        mic_.reset();
        mim_.reset(minput_open_im(data->language(), data->name(), nullptr));

        mplist_put(mim_->driver.callback_list, Minput_get_surrounding_text,
                   reinterpret_cast<void *>(&M17NState::callback));
        mplist_put(mim_->driver.callback_list, Minput_delete_surrounding_text,
                   reinterpret_cast<void *>(&M17NState::callback));
        mic_.reset(minput_create_ic(mim_.get(), this));
    }

    if (this->keyEvent(keyEvent.rawKey())) {
        keyEvent.filterAndAccept();
    }
}

bool M17NState::keyEvent(const Key &key) {
    if (!mic_) {
        return false;
    }
    MSymbol msym = KeySymToSymbol(key);

    if (msym == Mnil) {
        FCITX_M17N_DEBUG() << key << " not my dish";
        return false;
    }
    int thru = 0;
    if (!minput_filter(mic_.get(), msym, nullptr)) {
        MText *produced = mtext();
        // If input symbol was let through by m17n, let Fcitx handle it.
        // m17n may still produce some text to commit, though.
        thru = minput_lookup(mic_.get(), msym, NULL, produced);
        if (mtext_len(produced) > 0) {
            auto buf = MTextToUTF8(produced);
            ic_->commitString(buf);
        }
        m17n_object_unref(produced);
    }

    updateUI();

    return !thru;
}

void M17NState::updateUI() {
    ic_->inputPanel().reset();
    if (mic_->preedit) {
        auto preedit = MTextToUTF8(mic_->preedit);
        if (!preedit.empty()) {
            SetPreedit(ic_, preedit, mic_->cursor_pos);
        }
        FCITX_M17N_DEBUG() << "IM preedit changed to " << preedit;
    }
    ic_->updatePreedit();

    if (mic_->status) {
        auto mstatus = MTextToUTF8(mic_->status);
        // toShow = toShow || (strlen(mstatus) != 0);
        if (!mstatus.empty()) {
            FCITX_M17N_DEBUG() << "IM status changed to " << mstatus;
        }
    }
    if (mic_->candidate_list && mic_->candidate_show) {
        auto candList = std::make_unique<M17NCandidateList>(engine_, ic_);
        if (candList->size()) {
            candList->setGlobalCursorIndex(mic_->candidate_index);
            ic_->inputPanel().setCandidateList(std::move(candList));
        }
    }
    ic_->updateUserInterface(UserInterfaceComponent::InputPanel);
}

void M17NState::reset() {
    if (!mic_) {
        return;
    }
    minput_reset_ic(mic_.get());
    updateUI();
}

void M17NState::commitPreedit() {
    if (!mic_) {
        return;
    }

    if (!mic_->preedit) {
        return;
    }
    auto preedit = MTextToUTF8(mic_->preedit);
    if (!preedit.empty()) {
        ic_->commitString(preedit);
    }
}

void M17NState::select(int index) {
    if (!mic_) {
        return;
    }

    int lastIdx = mic_->candidate_index;
    do {
        if (index == mic_->candidate_index) {
            break;
        }
        if (index > mic_->candidate_index) {
            keyEvent(Key(FcitxKey_Right));
        } else if (index < mic_->candidate_index) {
            keyEvent(Key(FcitxKey_Left));
        }
        /* though useless, but take care if there is a bug cause freeze */
        if (lastIdx == mic_->candidate_index)
            break;
        lastIdx = mic_->candidate_index;
    } while (mic_->candidate_list && mic_->candidate_show);

    if (!mic_->candidate_list || !mic_->candidate_show ||
        index != mic_->candidate_index)
        return;

    MPlist *head = mic_->candidate_list;

    int i = 0;
    while (1) {
        int len;
        if (mplist_key(head) == Mtext) {
            len = mtext_len((MText *)mplist_value(head));
        } else {
            len = mplist_length((MPlist *)mplist_value(head));
        }

        if (i + len > index) {
            break;
        }

        i += len;
        head = mplist_next(head);
    }

    int delta = index - i;

    KeySym sym = FcitxKey_1;
    if ((delta + 1) % 10 == 0)
        sym = FcitxKey_0;
    else
        sym = static_cast<KeySym>(FcitxKey_1 + (delta % 10));
    keyEvent(Key(sym));
}
} // namespace fcitx

FCITX_ADDON_FACTORY(fcitx::M17NEngineFactory)
