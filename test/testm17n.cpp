/*
 * SPDX-FileCopyrightText: 2021-2021 CSSlayer <wengxt@gmail.com>
 *
 * SPDX-License-Identifier: LGPL-2.1-or-later
 *
 */
#include "testdir.h"
#include "testfrontend_public.h"
#include <fcitx-utils/eventdispatcher.h>
#include <fcitx-utils/log.h>
#include <fcitx-utils/standardpath.h>
#include <fcitx-utils/testing.h>
#include <fcitx/addonmanager.h>
#include <fcitx/inputcontextmanager.h>
#include <fcitx/inputmethodmanager.h>
#include <fcitx/instance.h>
#include <iostream>
#include <thread>

using namespace fcitx;

void scheduleEvent(EventDispatcher *dispatcher, Instance *instance) {
    dispatcher->schedule([dispatcher, instance]() {
        auto *m17n = instance->addonManager().addon("m17n", true);
        FCITX_ASSERT(m17n);
        auto defaultGroup = instance->inputMethodManager().currentGroup();
        defaultGroup.inputMethodList().clear();
        defaultGroup.inputMethodList().push_back(
            InputMethodGroupItem("keyboard-us"));
        defaultGroup.inputMethodList().push_back(
            InputMethodGroupItem("m17n_si_wijesekera"));
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

        dispatcher->schedule([dispatcher, instance]() {
            dispatcher->detach();
            instance->exit();
        });
    });
}

void runInstance() {}

int main() {
    setupTestingEnvironment(TESTING_BINARY_DIR, {TESTING_BINARY_DIR "/im"},
                            {TESTING_BINARY_DIR "/test"});
    // fcitx::Log::setLogRule("default=5,table=5,libime-table=5");
    char arg0[] = "testm17n";
    char arg1[] = "--disable=all";
    char arg2[] = "--enable=testim,testfrontend,m17n,testui";
    char *argv[] = {arg0, arg1, arg2};
    fcitx::Log::setLogRule("default=5,m17n=5");
    Instance instance(FCITX_ARRAY_SIZE(argv), argv);
    instance.addonManager().registerDefaultLoader(nullptr);
    EventDispatcher dispatcher;
    dispatcher.attach(&instance.eventLoop());
    std::thread thread(scheduleEvent, &dispatcher, &instance);
    instance.exec();
    thread.join();

    return 0;
}
