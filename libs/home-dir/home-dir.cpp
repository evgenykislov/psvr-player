/*
MIT License

Copyright (c) 2024 Evgeny Kislov

Permission is hereby granted, free of charge, to any person obtaining a copy
of this software and associated documentation files (the "Software"), to deal
in the Software without restriction, including without limitation the rights
to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
copies of the Software, and to permit persons to whom the Software is
furnished to do so, subject to the following conditions:

The above copyright notice and this permission notice shall be included in all
copies or substantial portions of the Software.

THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
SOFTWARE.
*/

// Detect platform
#if defined(linux) || defined(__linux) || defined(__linux__)
#define LINUX_PLATFORM
#elif defined(_WIN32) || defined(__WIN32__) || defined(WIN32)
#define WINDOWS_PLATFORM
#elif defined(macintosh) || defined(__APPLE__) || defined(__APPLE_CC__)
#define MACOS_PLATFORM
#else
#error Unknown platform
#endif

#include "home-dir.h"

// --------------------------
// Linux implementation
// --------------------------

#ifdef LINUX_PLATFORM

#include <string>

#include <pwd.h>
#include <stdlib.h>
#include <sys/types.h>
#include <unistd.h>

const char kHomeEnvVar[] = "HOME";
const char kConfigDir[] = "/.config";

std::string HomeDirLibrary::GetHomeDir() {
  try {
    auto ev = getenv(kHomeEnvVar);
    if (ev) {
      return std::string(ev);
    }
    auto pw = getpwuid(getuid());
    if (pw) {
      return std::string(pw->pw_dir);
    }
  } catch (std::exception) {
  }

  return std::string();
}


std::string HomeDirLibrary::GetDataDir() {
  return GetHomeDir() + kConfigDir;
}


#endif


// --------------------------
// Windows implementation
// --------------------------

#ifdef WINDOWS_PLATFORM

#include <windows.h>
#include <shlobj.h>

std::string GetCsidlPathA(int csidl) {
  try {
    CHAR s[MAX_PATH];
    if (SHGetFolderPathA(nullptr, csidl, nullptr, 0, s) == S_OK) {
      return std::string(s);
    }
  } catch (std::exception) {}
  return std::string();
}

std::wstring GetCsidlPathW(int csidl) {
  try {
    WCHAR s[MAX_PATH];
    if (SHGetFolderPathW(nullptr, csidl, nullptr, 0, s) == S_OK) {
      return std::wstring(s);
    }
  } catch (std::exception) {}
  return std::wstring();
}


std::string HomeDirLibrary::GetHomeDir() {
  return GetCsidlPathA(CSIDL_PROFILE);
}


std::wstring HomeDirLibrary::GetHomeDirW() {
  return GetCsidlPathW(CSIDL_PROFILE);
}


std::string HomeDirLibrary::GetDataDir() {
  return GetCsidlPathA(CSIDL_LOCAL_APPDATA);
}


std::wstring HomeDirLibrary::GetDataDirW() {
  return GetCsidlPathW(CSIDL_LOCAL_APPDATA);
}

#endif
