#include <fcitx-utils/uthash.h>
#include <fcitx-config/xdg.h>
#include <fcitx/ime.h>
#include "overrideparser.h"

static void OverrideItemFree(void* a);

static const UT_icd oil_icd = {sizeof(OverrideItem), 0, 0, OverrideItemFree};

void OverrideItemFree(void* a)
{
    OverrideItem* i = (OverrideItem*) a;
    free(i->name);
    fcitx_utils_free(i->i18nName);
}

int OverrideItemCmp(const void* a, const void* b)
{
    OverrideItem* ia = (OverrideItem*) a;
    OverrideItem* ib = (OverrideItem*) b;
    return ia->wildcardCount - ib->wildcardCount;
}

OverrideItemList* ParseDefaultSettings(FILE* fp)
{
    char* buf = NULL;
    size_t bufsize = 0;
    OverrideItemList* list = NULL;
    utarray_new(list, &oil_icd);
    while(getline(&buf, &bufsize, fp) != -1) {
        /* ignore comments */
        if (!buf || buf[0] == '#')
            continue;
        char* trimmed = fcitx_utils_trim(buf);
        UT_array* strList = fcitx_utils_split_string(trimmed, ':');
        free(trimmed);

        do {
            if (utarray_len(strList) < 3)
                break;
            char* lang = *(char**) utarray_eltptr(strList, 0);
            char* name = *(char**) utarray_eltptr(strList, 1);
            char* sPriority = *(char**) utarray_eltptr(strList, 2);
            char* i18nname = NULL;
            if (utarray_len(strList) == 4) {
                i18nname = *(char**) utarray_eltptr(strList, 3);
            }

            if (strlen(lang) > LANGCODE_LENGTH)
                break;
            int priority = atoi(sPriority);

            utarray_extend_back(list);
            OverrideItem* item = (OverrideItem*) utarray_back(list);
            strncpy(item->lang, lang, LANGCODE_LENGTH);
            item->name = strdup(name);
            item->priority = priority;
            item->i18nName = i18nname? strdup(i18nname) : 0;
            item->wildcardCount = 0;

            if (item->name[0] == '*')
                item->wildcardCount |= 1;

            if (item->lang[0] == '*')
                item->wildcardCount |= 2;
        } while (0);
        fcitx_utils_free_string_list(strList);
    }
    fcitx_utils_free(buf);

    utarray_sort(list, OverrideItemCmp);

    return list;
}

OverrideItem* MatchDefaultSettings(OverrideItemList* list, const char* lang, const char* name)
{
    OverrideItem* item;
    for (item = (OverrideItem*) utarray_front(list);
         item != NULL;
         item = (OverrideItem*) utarray_next(list, item))
    {
        if (!((item->wildcardCount & 2) || strcmp(lang, item->lang) == 0))
            continue;
        if (!((item->wildcardCount & 1) || strcmp(name, item->name) == 0))
            continue;
        break;
    }
    return item;
}
