/* replacement of minput_list, which is available in m17n-lib 1.6.2+ (CVS) */

#include <m17n.h>

MPlist *
minput_list (MSymbol language)
{
    MPlist *plist, *pl;
    MPlist *imlist = mplist ();

    plist = mdatabase_list (msymbol("input-method"), language, Mnil, Mnil);
    if (! plist)
        return imlist;
    for (pl = plist; pl && mplist_key(pl) != Mnil; pl = mplist_next (pl))
    {
        MDatabase *mdb = mplist_value (pl);
        MSymbol *tag = mdatabase_tag (mdb);
        MPlist *elm;

        elm = mplist ();
        mplist_add (elm, Msymbol, tag[1]);
        mplist_add (elm, Msymbol, tag[2]);
        if (tag[1] != Mnil && tag[2] != Mnil) {
            mplist_add (elm, Msymbol, Mt);
        } else {
            mplist_add (elm, Msymbol, Mnil);
        }
        mplist_push (imlist, Mplist, elm);
        m17n_object_unref (elm);
    }
    m17n_object_unref (plist);
    return imlist;
}

