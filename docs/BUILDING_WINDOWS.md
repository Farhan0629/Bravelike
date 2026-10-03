# Windows build guide

Use Visual Studio 2022 Community or Build Tools with Desktop development with C++, MSVC v143 x64/x86, Windows SDK and CMake 3.24+. Run commands in Developer PowerShell for VS 2022. Git is required for cloning; Python/Node are optional supplemental test tools.

## Core

```powershell
cmake -S . -B out\core -G "Visual Studio 17 2022" -A x64 -DKINGFN_BUILD_TESTS=ON
cmake --build out\core --config Release
ctest --test-dir out\core -C Release --output-on-failure
```

## Sandboxed CEF browser

```powershell
cmake -S . -B out\sandbox -G "Visual Studio 17 2022" -A x64 -DKINGFN_ENABLE_CEF=ON -DUSE_SANDBOX=ON
cmake --build out\sandbox --config Release --target kingfn_browser
.\out\sandbox\Release\kingfn_browser.exe
```

CEF is pinned to 152.0.7+g83ffcba+chromium-152.0.7977.83, Windows64 minimal. The download helper verifies the upstream checksum. Internet is needed on the first configuration. Missing `bootstrap.exe` fails configuration. The official CEF M138+ bootstrap EXE loads the same-named client DLL and supplies sandbox context; merely setting no_sandbox=false in an ordinary EXE is not sufficient.

Required packaging includes the bootstrap `kingfn_browser.exe`, client `kingfn_browser.dll`, `libcef.dll`, `chrome_elf.dll`, `icudtl.dat`, `resources.pak`, all other copied CEF resources/locales, `config/blocklist.txt`, and `resources/home.html`. Keep the complete output directory. Before distributing, preserve CEF/third-party notices and choose an authorized project license. The upstream bootstrap's icon/version resources are not automatically replaced by DLL resources.

## GPU workaround

Default is normal Chromium GPU behavior. For a machine requiring software rendering:

```powershell
.\out\sandbox\Release\kingfn_browser.exe --gpu-compatibility
```

This is not automatic GPU recovery. Record the flag in bug reports.

## Development-only unsandboxed build

```powershell
cmake -S . -B out\dev -G "Visual Studio 17 2022" -A x64 -DKINGFN_ENABLE_CEF=ON -DUSE_SANDBOX=OFF
cmake --build out\dev --config Release --target kingfn_browser
```

Do not use unsandboxed builds as public release candidates. Use separate output directories to avoid stale EXE/DLL combinations between modes. Existing BRAVELIKE_* cache options import only when the corresponding KINGFN_* entry is absent; explicit KINGFN_* wins.

## Runtime validation

Compilation and packaged-file checks do not prove sandbox isolation or working GUI features. Follow [WINDOWS_VALIDATION.md](WINDOWS_VALIDATION.md). `KINGFNProfile` beside the executable stores persistent local browsing data; installation under a read-only directory can prevent profile creation. A profile migration to a per-user writable application directory remains future work.
