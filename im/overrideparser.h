/*
 * SPDX-FileCopyrightText: 2012~2017 CSSlayer <wengxt@gmail.com>
 *
 * SPDX-License-Identifier: LGPL-2.1-or-later
 *
 */
#ifndef _IM_OVERRIDEPARSER_H_
#define _IM_OVERRIDEPARSER_H_

#include <string>
#include <vector>

struct OverrideItem {
    std::string lang;
    std::string name;
    int priority;
    std::string i18nName;
    int wildcardCount;
};

std::vector<OverrideItem> ParseDefaultSettings(FILE *fp);
const OverrideItem *MatchDefaultSettings(const std::vector<OverrideItem> &list,
                                         const std::string &lang,
                                         const std::string &name);

#endif // _IM_OVERRIDEPARSER_H_
