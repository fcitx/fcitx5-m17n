#ifndef OVERRIDEPARSER_H
#define OVERRIDEPARSER_H

#include <fcitx/ime.h>

typedef struct _OverrideList {
    char lang[LANGCODE_LENGTH + 1];
    char* name;
    int priority;
    char* i18nName;
    int wildcardCount;
} OverrideItem;

typedef UT_array OverrideItemList;

OverrideItemList* ParseDefaultSettings(FILE* fp);
OverrideItem* MatchDefaultSettings(OverrideItemList* list, const char* lang, const char* name);

#endif // OVERRIDEPARSER_H