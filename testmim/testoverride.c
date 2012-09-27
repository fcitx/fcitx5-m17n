#include "overrideparser.h"
#include <assert.h>

int main(int argc, char* argv[])
{
    FILE* fp = fopen(argv[1], "r");
    OverrideItemList* list =  ParseDefaultSettings(fp);

    OverrideItem* item = MatchDefaultSettings(list, "te", "phonetic");
    assert(item && strcmp("te", item->lang) == 0);

    utarray_free(list);
    fclose(fp);
    return 0;
}