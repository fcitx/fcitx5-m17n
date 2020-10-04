//
// Copyright (C) 2012~2017 by CSSlayer
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
