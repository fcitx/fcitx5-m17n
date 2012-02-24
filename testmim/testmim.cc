#include <string>
#include <sstream>
#include <string.h>
#include <locale.h>
#include "m17n.hh"

#define complain()\
    fprintf(stderr, "Bad things happened at %s:%d.\n", __FILE__, __LINE__)

enum { NLINE = 1024 };
char *mygetline(const char *prompt)
{
    static char line[NLINE];
    printf("%s", prompt);
    if (fgets(line, NLINE, stdin)) {
        int n = strlen(line);
        if (line[n - 1] == '\n') {
            line[n - 1] = '\0';
        }
        return line;
    } else {
        return NULL;
    }
}

int mtext_print(FILE *of, MText *text)
{
    size_t bufsize = (mtext_len(text) + 1) * 6;
    unsigned char *buf = new unsigned char[bufsize];

    m17n::Converter conv = mconv_buffer_converter(Mnil, buf, bufsize);
    mconv_encode(conv, text);
    buf[conv.id->nbytes] = '\0';

    int re = fprintf(of, "%s(%lu/%lu)", buf, strlen((char*)buf), bufsize);
    delete [] buf;
    return re;
}

int msymbol_print(FILE *of, MSymbol sym)
{
    return fprintf(of, "%s", msymbol_name(sym));
}

int mplist_print_text_values(FILE *of, MPlist *plist, const char *sep)
{
    int re = 0;
    re += fprintf(of, "[%d]", plist ? mplist_length(plist) : 0);
    bool first = true;
    for (; plist; plist = mplist_next(plist)) {
        if (!first) {
            re += fprintf(of, "%s", sep);
        }
        first = false;
        mtext_print(of, (MText*)mplist_value(plist));
    }
    return re;
}

int process_sym_stepped(MInputContext *ic, MSymbol sym)
{
    int re;
    if (minput_filter(ic, sym, NULL)) {
        printf("-%s:", msymbol_name(sym));
        re = 0;
    } else {
        m17n::Text converted = mtext();
        re = minput_lookup(ic, sym, NULL, converted);
        // Non-zero return values of minput_lookup() signify that the
        // character was not handled by the mim. According to libm17n
        // documentation, this is an error. But in practice this
        // should be interpreted as "let the character through";
        // The character shall be appended after ic->produced.

        printf("%s%s:", re ? "!" : "+", msymbol_name(sym));
        mtext_print(stdout, converted);
    }
    printf(":");
    mtext_print(stdout, ic->preedit);
#ifdef SHOW_CAND
    printf(":");
    if (ic->candidate_show && ic->candidate_list) {
        if (mplist_key(ic->candidate_list) == Mtext) {
            mtext_print(stdout, (MText*)mplist_value(ic->candidate_list));
        } else {
            mplist_print_text_values(stdout, ic->candidate_list, ",");
        }
    }
#endif
    printf("\n");
    return re;
}

int process_sym_unstepped(MInputContext *ic, MSymbol sym)
{
    int re;
    if (minput_filter(ic, sym, NULL)) {
        re = 0;
    } else {
        m17n::Text converted = mtext();
        re = minput_lookup(ic, sym, NULL, converted);
        mtext_print(stdout, converted);
    }
    if (re) {
        printf("%s", sym == Mnil ? "" : msymbol_name(sym));
    }
    // TODO Candidate list
    return re;
}

void list_mim() {
    // minput_list() returns a MPlist of MPlist's...
    MPlist *imlist = minput_list(Mnil);
    int n = mplist_length(imlist);
    printf("%d\n", n);
    for (; n--; imlist = mplist_next(imlist)) {
        MPlist *info = (MPlist*) mplist_value(imlist);

        MSymbol lang = (MSymbol) mplist_value(info);
        info = mplist_next(info);

        MSymbol name = (MSymbol) mplist_value(info);
        info = mplist_next(info);

        if (((MSymbol) mplist_value(info)) != Mt) {
            // Not sane
            continue;
        }

        msymbol_print(stdout, lang);
        printf(" ");
        msymbol_print(stdout, name);
        printf("\n");
    }
}

const char ps1[] = "\033[31m>\033[m ";
const char ps2[] = "\033[35m>>\033[m ";

typedef int (*process_func)(MInputContext*, MSymbol);
struct proc {
    process_func func;
    const char* header;
    const char* footer;
} procs[] = {
    {process_sym_stepped, "sym:converted:preedit\n", ""},
    {process_sym_unstepped, "-> ", "\n"},
};

#ifdef STEP
const proc* the_proc = &procs[0];
#else
const proc* the_proc = &procs[1];
#endif

int main()
{
    setlocale(LC_ALL, "");

    M17N_INIT();
    list_mim();

    char *line;
    while ((line = mygetline(ps1))) {
        std::istringstream ssym_line(line);
        std::string lang, name;
        if (!(ssym_line >> lang >> name && ssym_line.eof())) {
            fprintf(stderr, "Input format: $lang $name.  "
                    "Type EOF (CTRL-D) to exit.\n");
            continue;
        }
        m17n::Symbol mlang = msymbol(lang.c_str());
        m17n::Symbol mname = msymbol(name.c_str());
        if (!(mlang && mname)) {
            complain();
            continue;
        }
        m17n::InputMethod mim = minput_open_im(mlang, mname, 0);
        if (!mim) {
            fprintf(stderr, "Seems there is no such MIM.\n");
            continue;
        }

#ifdef SHOW_DESC
        // Show description
        m17n::Text mdesc = minput_get_description(mlang, mname);
        if (!mdesc) {
            complain();
            continue;
        }
        mtext_print(stdout, mdesc);
        printf("\n");
#endif

        // Create a context for a test session
        m17n::InputContext mic = minput_create_ic(mim, NULL);
        if (!mic) {
            fprintf(stderr, "Unable to create input context\n");
            continue;
        }
        fprintf(stderr, "Test session started.  Type emtpy line to exit.\n");
        while ((line = mygetline(ps2))) {
            if (line[0] == '\0') {
                fprintf(stderr, "End test session\n");
                break;
            }
            printf("%s", the_proc->header);
            const char *p;
            char buf[2] = " ";
            for (p = line; *p; p++) {
                buf[0] = *p;
                m17n::Symbol sym_char = msymbol(buf);
                the_proc->func(mic, sym_char);
            }
            the_proc->func(mic, Mnil);
            printf("%s", the_proc->footer);
            minput_reset_ic(mic);
        }
    }
    fprintf(stderr, "\n");

    M17N_FINI();
    return 0;
}

