//
// Copyright (C) 2017~2017 by CSSlayer
// wengxt@gmail.com
//
// This library is free software; you can redistribute it and/or modify
// it under the terms of the GNU Lesser General Public License as
// published by the Free Software Foundation; either version 2.1 of the
// License, or (at your option) any later version.
//
// This library is distributed in the hope that it will be useful,
// but WITHOUT ANY WARRANTY; without even the implied warranty of
// MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the GNU
// Lesser General Public License for more details.
//
// You should have received a copy of the GNU Lesser General Public
// License along with this library; see the file COPYING. If not,
// see <http://www.gnu.org/licenses/>.
//
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
