/*
 * SPDX-FileCopyrightText: 2021-2021 CSSlayer <wengxt@gmail.com>
 *
 * SPDX-License-Identifier: LGPL-2.1-or-later
 *
 */
#include "testdir.h"
#include "testfrontend_public.h"
#include <fcitx-utils/eventdispatcher.h>
#include <fcitx-utils/key.h>
#include <fcitx-utils/keysym.h>
#include <fcitx-utils/log.h>
#include <fcitx-utils/macros.h>
#include <fcitx-utils/testing.h>
#include <fcitx/addonmanager.h>
#include <fcitx/inputcontextmanager.h>
#include <fcitx/inputmethodgroup.h>
#include <fcitx/inputmethodmanager.h>
#include <fcitx/inputpanel.h>
#include <fcitx/instance.h>
#include <string>

using namespace fcitx;

void testWijesekara(Instance *instance) {
    instance->eventDispatcher().schedule([instance]() {
        auto *m17n = instance->addonManager().addon("m17n", true);
        FCITX_ASSERT(m17n);
        auto defaultGroup = instance->inputMethodManager().currentGroup();
        std::string wijesekaraName;
        if (instance->inputMethodManager().entry("m17n_si_wijesekera")) {
            wijesekaraName = "m17n_si_wijesekera";
        } else if (instance->inputMethodManager().entry("m17n_si_wijesekara")) {
            wijesekaraName = "m17n_si_wijesekara";
        } else {
            FCITX_ERROR()
                << "wijesekara engine is not available, skip the test";
            return;
        }
        defaultGroup.inputMethodList().clear();
        defaultGroup.inputMethodList().push_back(
            InputMethodGroupItem("keyboard-us"));
        defaultGroup.inputMethodList().push_back(
            InputMethodGroupItem(wijesekaraName));
        defaultGroup.setDefaultInputMethod("");
        instance->inputMethodManager().setGroup(defaultGroup);
        auto *testfrontend = instance->addonManager().addon("testfrontend");
        auto uuid =
            testfrontend->call<ITestFrontend::createInputContext>("testapp");
        auto ic = instance->inputContextManager().findByUUID(uuid);
        ic->focusIn();
        testfrontend->call<ITestFrontend::pushCommitExpectation>("ඳ");
        testfrontend->call<ITestFrontend::pushCommitExpectation>("ඟ");
        testfrontend->call<ITestFrontend::pushCommitExpectation>("ග");
        testfrontend->call<ITestFrontend::pushCommitExpectation>("ඤ");
        testfrontend->call<ITestFrontend::pushCommitExpectation>("ඥ");
        testfrontend->call<ITestFrontend::pushCommitExpectation>(":");
        testfrontend->call<ITestFrontend::keyEvent>(uuid, Key("Control+space"),
                                                    false);
        testfrontend->call<ITestFrontend::keyEvent>(uuid, Key("Alt+o"), false);
        testfrontend->call<ITestFrontend::keyEvent>(uuid, Key("Alt+period"),
                                                    false);
        testfrontend->call<ITestFrontend::keyEvent>(uuid, Key("period"), false);
        testfrontend->call<ITestFrontend::keyEvent>(
            uuid, Key(FcitxKey_bracketleft), false);
        testfrontend->call<ITestFrontend::keyEvent>(
            uuid, Key(FcitxKey_braceleft), false);
        testfrontend->call<ITestFrontend::keyEvent>(
            uuid, Key(FcitxKey_braceright), false);
        testfrontend->call<ITestFrontend::keyEvent>(uuid, Key("Control+space"),
                                                    false);
        delete ic;
    });
}

void testSwitchWithUnicode(Instance *instance) {
    instance->eventDispatcher().schedule(
        [instance]() {
            auto *m17n = instance->addonManager().addon("m17n", true);
            FCITX_ASSERT(m17n);
            auto defaultGroup = instance->inputMethodManager().currentGroup();
            std::string wijesekaraName;
            if (!instance->inputMethodManager().entry("m17n_zh_pinyin") ||
                !instance->inputMethodManager().entry("m17n_zh_bopomofo")) {
                FCITX_ERROR()
                    << "wijesekara engine is not available, skip the test";
                return;
            }
            defaultGroup.inputMethodList().clear();
            defaultGroup.inputMethodList().push_back(
                InputMethodGroupItem("keyboard-us"));
            defaultGroup.inputMethodList().push_back(
                InputMethodGroupItem("m17n_zh_pinyin"));
            defaultGroup.inputMethodList().push_back(
                InputMethodGroupItem("m17n_zh_bopomofo"));
            defaultGroup.setDefaultInputMethod("m17n_zh_pinyin");
            instance->inputMethodManager().setGroup(defaultGroup);
            auto *testfrontend = instance->addonManager().addon("testfrontend");
            auto uuid = testfrontend->call<ITestFrontend::createInputContext>(
                "testapp");
            auto ic = instance->inputContextManager().findByUUID(uuid);
            ic->focusIn();
            instance->activate();
            FCITX_ASSERT(instance->inputMethod(ic) == "m17n_zh_pinyin");
            testfrontend->call<ITestFrontend::keyEvent>(uuid, Key("Control+u"),
                                                        false);
            const auto commitPreedit = ic->inputPanel().preedit().toString();
            if (!commitPreedit.empty()) {
                testfrontend->call<ITestFrontend::pushCommitExpectation>(
                    commitPreedit);
            }
            instance->setCurrentInputMethod(ic, "m17n_zh_bopomofo", false);
            FCITX_ASSERT(instance->inputMethod(ic) == "m17n_zh_bopomofo");
            testfrontend->call<ITestFrontend::pushCommitExpectation>("ａ");
            testfrontend->call<ITestFrontend::keyEvent>(uuid, Key("a"), false);
            delete ic;
        });
}

int main() {
    setupTestingEnvironment(TESTING_BINARY_DIR, {"bin"},
                            {TESTING_BINARY_DIR "/test"});
    // fcitx::Log::setLogRule("default=5,table=5,libime-table=5");
    char arg0[] = "testm17n";
    char arg1[] = "--disable=all";
    char arg2[] = "--enable=testim,testfrontend,m17n,testui";
    char *argv[] = {arg0, arg1, arg2};
    fcitx::Log::setLogRule("default=5,m17n=5");
    Instance instance(FCITX_ARRAY_SIZE(argv), argv);
    instance.addonManager().registerDefaultLoader(nullptr);
    testWijesekara(&instance);
    testSwitchWithUnicode(&instance);
    instance.eventDispatcher().schedule([&instance]() { instance.exit(); });
    instance.exec();

    return 0;
}
