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
#ifndef _IM_ENGINE_H_
#define _IM_ENGINE_H_

#include "overrideparser.h"
#include <fcitx-config/iniparser.h>
#include <fcitx-utils/i18n.h>
#include <fcitx/addonfactory.h>
#include <fcitx/addonmanager.h>
#include <fcitx/inputcontextproperty.h>
#include <fcitx/inputmethodengine.h>
#include <m17n.h>

namespace fcitx {

FCITX_CONFIGURATION(M17NConfig, Option<bool> enableDeprecated{
                                    this, "EnableDeprecated",
                                    _("Enable Deprecated"), false};);

class M17NData : public InputMethodEntryUserData {
public:
    M17NData(MSymbol language, MSymbol name)
        : language_(language), name_(name) {}

    MSymbol language() const { return language_; }
    MSymbol name() const { return name_; }

private:
    MSymbol language_;
    MSymbol name_;
};

class M17NEngine;

class M17NState : public InputContextProperty {
public:
    M17NState(M17NEngine *engine, InputContext *ic)
        : engine_(engine), ic_(ic), mim_(nullptr, &minput_close_im),
          mic_(nullptr, &minput_destroy_ic) {}

    void keyEvent(const InputMethodEntry &entry, KeyEvent &keyEvent);
    void command(MInputContext *context, MSymbol command);
    void updateUI();
    void select(int index);
    void reset();
    bool keyEvent(const Key &key);

    static void callback(MInputContext *context, MSymbol command);

    M17NEngine *engine_;
    InputContext *ic_;
    std::unique_ptr<MInputMethod, decltype(&minput_close_im)> mim_;
    std::unique_ptr<MInputContext, decltype(&minput_destroy_ic)> mic_;
};

class M17NEngine : public InputMethodEngine {
public:
    M17NEngine(Instance *instance);

    void activate(const fcitx::InputMethodEntry &,
                  fcitx::InputContextEvent &) override;
    void keyEvent(const fcitx::InputMethodEntry &entry,
                  fcitx::KeyEvent &keyEvent) override;
    void reloadConfig() override;
    void reset(const fcitx::InputMethodEntry &,
               fcitx::InputContextEvent &) override;
    auto factory() { return &factory_; }

    const Configuration *getConfig() const override { return &config_; }
    void setConfig(const RawConfig &config) override {
        config_.load(config, true);
        safeSaveAsIni(config_, "conf/m17n.conf");
    }

    std::vector<InputMethodEntry> listInputMethods() override;

private:
    Instance *instance_;
    M17NConfig config_;
    std::vector<OverrideItem> list_;
    FactoryFor<M17NState> factory_;
};

class M17NEngineFactory : public AddonFactory {
    fcitx::AddonInstance *create(fcitx::AddonManager *manager) override {
        return new M17NEngine(manager->instance());
    }
};
} // namespace fcitx

#endif // _IM_ENGINE_H_
