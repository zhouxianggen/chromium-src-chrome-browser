// Copyright 2013 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "chrome/browser/android/password_ui_view_android.h"

#include "base/android/jni_helper.h"
#include "base/android/jni_string.h"
#include "chrome/browser/profiles/profile_manager.h"
#include "components/autofill/core/common/password_form.h"
#include "jni/PasswordUIView_jni.h"

using base::android::ConvertUTF16ToJavaString;
using base::android::ConvertUTF8ToJavaString;
using base::android::ScopedJavaLocalRef;

PasswordUIViewAndroid::PasswordUIViewAndroid(JNIEnv* env, jobject obj)
    : password_manager_presenter_(this), weak_java_ui_controller_(env, obj) {}

PasswordUIViewAndroid::~PasswordUIViewAndroid() {}

void PasswordUIViewAndroid::Destroy(JNIEnv*, jobject) { delete this; }

Profile* PasswordUIViewAndroid::GetProfile() {
  return ProfileManager::GetLastUsedProfile();
}

void PasswordUIViewAndroid::ShowPassword(
    size_t index, const string16& password_value) {
  NOTIMPLEMENTED();
}

void PasswordUIViewAndroid::SetPasswordList(
    const ScopedVector<autofill::PasswordForm>& password_list,
    bool show_passwords) {
  // Android just ignores the |show_passwords| argument.
  JNIEnv* env = base::android::AttachCurrentThread();
  ScopedJavaLocalRef<jobject> ui_controller = weak_java_ui_controller_.get(env);
  if (!ui_controller.is_null()) {
    Java_PasswordUIView_passwordListAvailable(
        env, ui_controller.obj(), static_cast<int>(password_list.size()));
  }
}

void PasswordUIViewAndroid::SetPasswordExceptionList(
    const ScopedVector<autofill::PasswordForm>& password_exception_list) {
  JNIEnv* env = base::android::AttachCurrentThread();
  ScopedJavaLocalRef<jobject> ui_controller = weak_java_ui_controller_.get(env);
  if (!ui_controller.is_null()) {
    Java_PasswordUIView_passwordExceptionListAvailable(
        env,
        ui_controller.obj(),
        static_cast<int>(password_exception_list.size()));
  }
}

void PasswordUIViewAndroid::UpdatePasswordLists(JNIEnv* env, jobject) {
  password_manager_presenter_.UpdatePasswordLists();
}

ScopedJavaLocalRef<jobject>
PasswordUIViewAndroid::GetSavedPasswordEntry(JNIEnv* env, jobject, int index) {
  const autofill::PasswordForm& form =
      password_manager_presenter_.GetPassword(index);
  return Java_PasswordUIView_createSavedPasswordEntry(
      env,
      ConvertUTF8ToJavaString(env, form.origin.spec()).obj(),
      ConvertUTF16ToJavaString(env, form.username_value).obj());
}

ScopedJavaLocalRef<jstring> PasswordUIViewAndroid::GetSavedPasswordException(
    JNIEnv* env, jobject, int index) {
  const autofill::PasswordForm& form =
      password_manager_presenter_.GetPasswordException(index);
  return ConvertUTF8ToJavaString(env, form.origin.spec());
}

void PasswordUIViewAndroid::HandleRemoveSavedPasswordEntry(
    JNIEnv* env, jobject, int index) {
  password_manager_presenter_.RemoveSavedPassword(index);
}

void PasswordUIViewAndroid::HandleRemoveSavedPasswordException(
    JNIEnv* env, jobject, int index) {
  password_manager_presenter_.RemovePasswordException(index);
}

// static
static jlong Init(JNIEnv* env, jobject obj) {
  PasswordUIViewAndroid* controller = new PasswordUIViewAndroid(env, obj);
  return reinterpret_cast<intptr_t>(controller);
}

bool PasswordUIViewAndroid::RegisterPasswordUIViewAndroid(JNIEnv* env) {
  return RegisterNativesImpl(env);
}
