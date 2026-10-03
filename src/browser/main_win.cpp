#include <filesystem>
#include <string>
#include <windows.h>
#include <shellapi.h>

#include "include/cef_app.h"
#include "src/browser/browser_app.h"
#include "src/core/navigation.h"

namespace {
std::wstring ProfilePath() {
  wchar_t path[MAX_PATH]{};
  const DWORD length = GetModuleFileNameW(nullptr, path, MAX_PATH);
  if (length == 0 || length == MAX_PATH) return L"BravelikeProfile";
  return (std::filesystem::path(path).parent_path() / L"BravelikeProfile").wstring();
}
}

int APIENTRY wWinMain(HINSTANCE instance, HINSTANCE previous_instance,
                      wchar_t* command_line, int show_command) {
  UNREFERENCED_PARAMETER(previous_instance);
  UNREFERENCED_PARAMETER(command_line);
  UNREFERENCED_PARAMETER(show_command);

  CefMainArgs main_args(instance);
  void* sandbox_info = nullptr;
  const int subprocess_code = CefExecuteProcess(main_args, nullptr, sandbox_info);
  if (subprocess_code >= 0) return subprocess_code;

  CefSettings settings;
  settings.no_sandbox = true;
  settings.persist_session_cookies = true;
  CefString(&settings.cache_path) = ProfilePath();
  CefString(&settings.user_agent_product) = L"Bravelike/0.2";

  std::string startup_url;
  if (command_line && wcslen(command_line) > 0) {
    int argc = 0;
    wchar_t** argv = CommandLineToArgvW(command_line, &argc);
    if (argv) {
      for (int i = 0; i < argc; ++i) {
        std::wstring arg = argv[i];
        if (arg.rfind(L"--", 0) != 0 && arg.rfind(L"-", 0) != 0) {
          std::string str(arg.begin(), arg.end());
          startup_url = bravelike::ResolveAddressInput(str);
          break;
        }
      }
      LocalFree(argv);
    }
  }

  CefRefPtr<bravelike::BrowserApp> app = new bravelike::BrowserApp(startup_url);
  if (!CefInitialize(main_args, settings, app, sandbox_info)) return 1;
  CefRunMessageLoop();
  CefShutdown();
  return 0;
}
