// Copyright (c) 2012 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "chrome/browser/ui/webui/options/chromeos/keyboard_handler.h"

#include "base/command_line.h"
#include "base/values.h"
#include "chrome/browser/chromeos/input_method/xkeyboard.h"
#include "chrome/common/chrome_switches.h"
#include "content/public/browser/web_ui.h"
#include "grit/generated_resources.h"
#include "ui/base/l10n/l10n_util.h"

namespace {
const struct ModifierKeysSelectItem {
  int message_id;
  chromeos::input_method::ModifierKey value;
} kModifierKeysSelectItems[] = {
  { IDS_OPTIONS_SETTINGS_LANGUAGES_KEY_SEARCH,
    chromeos::input_method::kSearchKey },
  { IDS_OPTIONS_SETTINGS_LANGUAGES_KEY_LEFT_CTRL,
    chromeos::input_method::kControlKey },
  { IDS_OPTIONS_SETTINGS_LANGUAGES_KEY_LEFT_ALT,
    chromeos::input_method::kAltKey },
  { IDS_OPTIONS_SETTINGS_LANGUAGES_KEY_VOID,
    chromeos::input_method::kVoidKey },
  { IDS_OPTIONS_SETTINGS_LANGUAGES_KEY_CAPS_LOCK,
    chromeos::input_method::kCapsLockKey },
};

const char* kDataValuesNames[] = {
  "remapSearchKeyToValue",
  "remapControlKeyToValue",
  "remapAltKeyToValue",
  "remapCapsLockKeyToValue",
};
}  // namespace

namespace chromeos {
namespace options {

KeyboardHandler::KeyboardHandler() {
}

KeyboardHandler::~KeyboardHandler() {
}

void KeyboardHandler::GetLocalizedValues(DictionaryValue* localized_strings) {
  DCHECK(localized_strings);

  localized_strings->SetString("keyboardOverlayTitle",
      l10n_util::GetStringUTF16(IDS_OPTIONS_KEYBOARD_OVERLAY_TITLE));
  localized_strings->SetString("remapSearchKeyToContent",
      l10n_util::GetStringUTF16(
          IDS_OPTIONS_SETTINGS_LANGUAGES_KEY_SEARCH_LABEL));
  localized_strings->SetString("remapControlKeyToContent",
      l10n_util::GetStringUTF16(
          IDS_OPTIONS_SETTINGS_LANGUAGES_KEY_LEFT_CTRL_LABEL));
  localized_strings->SetString("remapAltKeyToContent",
      l10n_util::GetStringUTF16(
          IDS_OPTIONS_SETTINGS_LANGUAGES_KEY_LEFT_ALT_LABEL));
  localized_strings->SetString("remapCapsLockKeyToContent",
      l10n_util::GetStringUTF16(
          IDS_OPTIONS_SETTINGS_LANGUAGES_KEY_CAPS_LOCK_LABEL));
  localized_strings->SetString("searchKeyActsAsFunctionKey",
      l10n_util::GetStringUTF16(
          IDS_OPTIONS_SETTINGS_LANGUAGES_KEY_SEARCH_AS_FUNCTION));
  localized_strings->SetString("changeLanguageAndInputSettings",
      l10n_util::GetStringUTF16(
          IDS_OPTIONS_SETTINGS_CHANGE_LANGUAGE_AND_INPUT_SETTINGS));

  for (size_t i = 0; i < arraysize(kDataValuesNames); ++i) {
    ListValue* list_value = new ListValue();
    for (size_t j = 0; j < arraysize(kModifierKeysSelectItems); ++j) {
      const input_method::ModifierKey value =
          kModifierKeysSelectItems[j].value;
      const int message_id = kModifierKeysSelectItems[j].message_id;
      // Only the seach key can be remapped to the caps lock key.
      if (kDataValuesNames[i] != std::string("remapSearchKeyToValue") &&
          kDataValuesNames[i] != std::string("remapCapsLockKeyToValue") &&
          value == input_method::kCapsLockKey) {
        continue;
      }
      ListValue* option = new ListValue();
      option->Append(Value::CreateIntegerValue(value));
      option->Append(Value::CreateStringValue(l10n_util::GetStringUTF16(
          message_id)));
      list_value->Append(option);
    }
    localized_strings->Set(kDataValuesNames[i], list_value);
  }
}

void KeyboardHandler::InitializePage() {
  bool chromeos_keyboard = CommandLine::ForCurrentProcess()->HasSwitch(
      switches::kHasChromeOSKeyboard);
  bool chromebook_function_key = CommandLine::ForCurrentProcess()->HasSwitch(
      switches::kEnableChromebookFunctionKey);

  const base::FundamentalValue show_options(true);

  if (!chromeos_keyboard) {
    web_ui()->CallJavascriptFunction(
        "options.KeyboardOverlay.showCapsLockOptions", show_options);
  }

  if (chromeos_keyboard && chromebook_function_key) {
    web_ui()->CallJavascriptFunction(
        "options.KeyboardOverlay.showFunctionKeyOptions", show_options);
  }
}

}  // namespace options
}  // namespace chromeos
