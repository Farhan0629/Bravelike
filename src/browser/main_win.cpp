#include <filesystem>
#include <string>
#include <windows.h>
#include "include/cef_app.h"
#include "include/cef_command_line.h"
#include "include/cef_sandbox_win.h"
#include "src/browser/browser_app.h"
#include "src/core/navigation.h"

namespace {
std::wstring ProfilePath() {
  wchar_t path[MAX_PATH]{};
  const DWORD length = GetModuleFileNameW(nullptr, path, MAX_PATH);
  if (length == 0 || length == MAX_PATH) return L"KINGFNProfile";
  return (std::filesystem::path(path).parent_path() / L"KINGFNProfile").wstring();
}
int RunMain(HINSTANCE instance, void* sandbox_info) {
  CefMainArgs main_args(instance);
  const int subprocess_code = CefExecuteProcess(main_args, nullptr, sandbox_info);
  if (subprocess_code >= 0) return subprocess_code;
  CefSettings settings;
#ifdef KINGFN_USE_BOOTSTRAP
  // Fail closed if the bootstrap did not supply the sandbox context.
  if (!sandbox_info) return 1;
  settings.no_sandbox = false;
#else
  settings.no_sandbox = true;
#endif
  settings.persist_session_cookies = true;
  CefString(&settings.cache_path) = ProfilePath();
  CefString(&settings.user_agent_product) = L"KINGFN/0.2";
  auto cmd = CefCommandLine::CreateCommandLine();
  cmd->InitFromString(GetCommandLineW());
#ifdef KINGFN_USE_BOOTSTRAP
  if (cmd->HasSwitch("no-sandbox")) return 1;
#endif
  CefCommandLine::ArgumentList args;
  cmd->GetArguments(args);
  std::string startup_url;
  for (const auto& arg : args) {
    const auto value = arg.ToString();
    if (!value.empty() && value.front() != '-') {
      startup_url = kingfn::ResolveAddressInput(value);
      break;
    }
  }
  CefRefPtr<kingfn::BrowserApp> app = new kingfn::BrowserApp(startup_url);
  if (!CefInitialize(main_args, settings, app, sandbox_info)) return 1;
  CefRunMessageLoop();
  CefShutdown();
  return 0;
}
}
#ifdef KINGFN_USE_BOOTSTRAP
CEF_BOOTSTRAP_EXPORT int RunWinMain(HINSTANCE instance, LPWSTR command_line,
    int show_command, void* sandbox_info, cef_version_info_t* version_info) {
  return RunMain(instance, sandbox_info);
}
#else
int APIENTRY wWinMain(HINSTANCE instance, HINSTANCE previous_instance,
    wchar_t* command_line, int show_command) {
  return RunMain(instance, nullptr);
}
#endif
