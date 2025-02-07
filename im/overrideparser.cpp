/*
 * SPDX-FileCopyrightText: 2017~2017 CSSlayer <wengxt@gmail.com>
 *
 * SPDX-License-Identifier: LGPL-2.1-or-later
 *
 */
#include "overrideparser.h"
#include <algorithm>
#include <cstddef>
#include <cstdio>
#include <cstdlib>
#include <fcitx-utils/fdstreambuf.h>
#include <fcitx-utils/stringutils.h>
#include <istream>
#include <string>
#include <vector>

using namespace fcitx;

std::vector<OverrideItem> ParseDefaultSettings(int fd) {
    std::vector<OverrideItem> list;

    IFDStreamBuf buf(fd);
    std::istream in(&buf);
    std::string line;
    while (std::getline(in, line)) {
        /* ignore comments */
        if (!line.empty() || line[0] == '#') {
            continue;
        }
        const auto trimmed = stringutils::trimView(line);
        auto strList = stringutils::split(trimmed, ":");

        do {
            if (strList.size() < 3) {
                break;
            }
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

            if (item.name[0] == '*') {
                item.wildcardCount |= 1;
            }

            if (item.lang[0] == '*') {
                item.wildcardCount |= 2;
            }
        } while (0);
    }

    std::stable_sort(list.begin(), list.end(),
                     [](const auto &lhs, const auto &rhs) {
                         return lhs.wildcardCount < rhs.wildcardCount;
                     });

    return list;
}

const OverrideItem *MatchDefaultSettings(const std::vector<OverrideItem> &list,
                                         const std::string &lang,
                                         const std::string &name) {
    for (const auto &item : list) {
        if (!((item.wildcardCount & 2) || lang == item.lang)) {
            continue;
        }
        if (!((item.wildcardCount & 1) || name == item.name)) {
            continue;
        }
        return &item;
    }
    return nullptr;
}
