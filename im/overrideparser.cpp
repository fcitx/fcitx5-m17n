/*
 * SPDX-FileCopyrightText: 2017~2017 CSSlayer <wengxt@gmail.com>
 *
 * SPDX-License-Identifier: LGPL-2.1-or-later
 *
 */
#include "overrideparser.h"
#include <fcitx-utils/stringutils.h>

using namespace fcitx;

std::vector<OverrideItem> ParseDefaultSettings(FILE *fp) {
    char *buf = NULL;
    size_t bufsize = 0;
    std::vector<OverrideItem> list;
    while (getline(&buf, &bufsize, fp) != -1) {
        /* ignore comments */
        if (!buf || buf[0] == '#')
            continue;
        auto trimmed = stringutils::trim(buf);
        auto strList = stringutils::split(trimmed, ":");

        do {
            if (strList.size() < 3)
                break;
            const auto &lang = strList[0];
            const auto &name = strList[1];
            const auto &sPriority = strList[2];

            int priority = std::stoi(sPriority);
            list.emplace_back();
            auto &item = list.back();
            item.lang = lang;
            item.name = name;
            item.priority = priority;
            item.i18nName = strList.size() == 4 ? strList[3] : "";
            item.wildcardCount = 0;

            if (item.name[0] == '*')
                item.wildcardCount |= 1;

            if (item.lang[0] == '*')
                item.wildcardCount |= 2;
        } while (0);
    }
    free(buf);

    std::stable_sort(list.begin(), list.end(),
                     [](const auto &lhs, const auto &rhs) {
                         return lhs.wildcardCount < rhs.wildcardCount;
                     });

    return list;
}

const OverrideItem *MatchDefaultSettings(const std::vector<OverrideItem> &list,
                                         const std::string &lang,
                                         const std::string &name) {
    for (auto &item : list) {
        if (!((item.wildcardCount & 2) || lang == item.lang))
            continue;
        if (!((item.wildcardCount & 1) || name == item.name))
            continue;
        return &item;
    }
    return nullptr;
}
