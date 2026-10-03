#include <filesystem>
#include <string>
#include <windows.h>
#include <shellapi.h>

#include "include/cef_app.h"
#include "include/cef_command_line.h"
#include "src/browser/browser_app.h"
#include "src/core/navigation.h"

namespace {
std::wstring ProfilePath() {
  wchar_t path[MAX_PATH]{};
  const DWORD length = GetModuleFileNameW(nullptr, path, MAX_PATH);
  if (length == 0 || length == MAX_PATH) return L"KINGFNProfile";
  return (std::filesystem::path(path).parent_path() / L"KINGFNProfile").wstring();
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
  CefString(&settings.user_agent_product) = L"KINGFN/0.2";

  CefRefPtr<CefCommandLine> cmd = CefCommandLine::CreateCommandLine();
  cmd->InitFromString(GetCommandLineW());
  CefCommandLine::ArgumentList args;
  cmd->GetArguments(args);

  std::string startup_url;
  for (const auto& arg : args) {
    const std::string str = arg.ToString();
    if (!str.empty() && str.rfind("--", 0) != 0 && str.rfind("-", 0) != 0) {
      startup_url = kingfn::ResolveAddressInput(str);
      break;
    }
  }

  CefRefPtr<kingfn::BrowserApp> app = new kingfn::BrowserApp(startup_url);
  if (!CefInitialize(main_args, settings, app, sandbox_info)) return 1;
  CefRunMessageLoop();
  CefShutdown();
  return 0;
}
