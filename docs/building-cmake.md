# Windows x86: CMake and Clang

The existing Visual Studio solution remains available. This workflow builds the same Windows-only game with clang-cl, Ninja, Microsoft link.exe, and Windows rc.exe. It does not port the game to another OS or CPU.

## Prerequisites

- Visual Studio 2022 Build Tools: C++ x86/x64 tools, MFC x86/x64, Windows 10/11 SDK, and C++ CMake tools (CMake 3.29 or newer and Ninja).
- LLVM 23.1.3 (the version validated for this workflow): `winget install --id LLVM.LLVM --exact --version 23.1.3`.
- Python 3, located by CMake's FindPython3 (not a hardcoded executable name).
- DirectX 8.1 SDK headers and libraries in `dx8/include` and `dx8/lib`, or pass `-Dx8SdkDir` below. This proprietary legacy SDK is not fetched by CMake or committed.
- Your Freelancer DATA directory for gameplay only. Building does not require it.

## Build and stage

From PowerShell in the repository root:

```powershell
./scripts/build.ps1 -Configuration Debug -DataDir C:\Freelancer\DATA
./scripts/build.ps1 -Configuration Release -DataDir C:\Freelancer\DATA
# An SDK elsewhere:
./scripts/build.ps1 -Configuration Debug -Dx8SdkDir C:\DXSDK
```

The wrapper discovers VS 2022 with vswhere, imports its x86 target environment, locates bundled CMake/Ninja, and invokes the committed presets. It never requires permanently editing PATH. Missing components produce errors; the wrapper does not install software automatically.

`-ConfigureOnly` validates prerequisites and generates the build. `-Clean` rebuilds all targets. `-SkipInstall` builds and tests without staging. Tests run automatically before staging.

In an already prepared x86 developer environment, with LLVM/CMake/Ninja on PATH, the equivalent commands are:

```powershell
cmake --preset windows-x86-clang-debug -DFL_DATA_DIR=C:/Freelancer/DATA
cmake --build --preset windows-x86-clang-debug
cmake --install out/build/windows-x86-clang-debug
```

Each preset has independent `out/build/<preset>` and `out/stage/<preset>` directories. Launch from the staged `bin` directory:

```powershell
Push-Location out/stage/windows-x86-clang-debug/bin
./freelancer.exe
Pop-Location
```

Staging copies runtime configuration with the existing `copy_dev_ini.py`, keeps DATA_EXTRA and retained retail files at their expected relative paths, and creates a directory junction named DATA to your installation. It does not modify or package the original DATA. An existing junction to another directory is rejected; remove that junction manually before changing FL_DATA_DIR. Without FL_DATA_DIR staging succeeds, but gameplay requires supplying DATA before launch.

Machine-specific settings can be kept in ignored CMakeUserPresets.json using a preset inheriting a shared preset. The wrapper operates the two shared presets; invoke custom presets directly in a prepared developer environment.

## Settings and compatibility

CMake cache settings: FL_DX8_SDK_DIR, FL_DATA_DIR, FL_WIDESCREEN (ON), FL_WALK_MODE (ON), FL_BETA_LABEL (OFF), FL_TICK_PROFILE (OFF), FL_RP9 (OFF), FL_DX9_SDK_DIR.

The CMake target graph has explicit import-library dependencies. Generated binaries do not link to the repository's existing bin/ libraries. DLL exports, resource IDs, C++17, /MD[d], and Release iterator debugging level 1 are retained. DirectX 8.1's obsolete libc/libci defaults are ignored for RP8, and SafeSEH is disabled there as in the existing solution.

## CI SDK provisioning

The Windows workflow builds Debug and Release on windows-2022, without DATA. Configure repository secret DX8_SDK_ARCHIVE_URL with a private, HTTPS-accessible ZIP containing the SDK's include/ and lib/ at its root, and repository variable DX8_SDK_ARCHIVE_SHA256 with its SHA-256. A missing secret (including untrusted fork PRs) or hash mismatch fails with a diagnostic. The SDK and game files are never uploaded as CI artifacts. This project does not supply or redistribute the proprietary SDK.

The runner must have the same VS 2022 C++/MFC/Windows SDK/CMake components as a local developer. LLVM is installed if the runner lacks it. Full gameplay testing is local and separate from CI compilation.


## Verification

The wrapper runs CTest before staging. Tests require no game DATA and cover x87 precision control, explicit SSE instructions, Unicode resources, all 27 FastDot kernels, quaternion-to-matrix conversion, quaternion interpolation/aliasing, spline reference operands, and loading a position spline (including endpoint ghost vectors).

The Clang port uses explicit pointer variables for C++ references passed into inline assembly. MSVC and Clang interpret bare reference operands differently; without the pointers, startup can dereference a quaternion/vector component as an address. The quaternion initialization failure can surface as an apparent Content.dll loading error even though the DLL exists.

Animation regression tests also cover all parameter-curve factories, scalar and extended results, event timing with and without repeating curves, light-color interpolation, audio interpolation, and x87 stack balance. Ordinary C++ functions must explicitly return assembly-produced floating-point results under Clang; leaving a value in ST(0) alone can return garbage and disrupt animation or lighting. The Clang-only return helpers preserve that register's value. Hermite segment selection uses a fixed operand block so its assembly cannot overwrite the compiler's frame-address register.

The shared x87 trigonometry helpers, reciprocal square root, and spline blend also have numeric regression coverage. An audit of Clang's parsed assembly-bearing functions in the active Windows x86 configuration found no further missing floating-point returns after the Thorn fixes. Naked assembly entry points deliberately return registers directly and are handled separately; this audit does not establish full gameplay correctness or cover inactive platform/configuration branches.

Debug staging includes development binaries; it requires the development runtimes installed by Visual Studio Build Tools. Staged runtime directories are not standalone release installers. CI compilation and these tests do not replace gameplay testing with a local installation.
