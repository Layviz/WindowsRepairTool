# Windows Repair Tool

A Windows 10/11 x64 console application that runs Windows' built-in repair tools.
Build with Visual Studio 2022 (Desktop development with C++, Windows SDK) or CMake
and a Windows C++17 toolchain. The existing solution and MSI setup project remain
available; MSI packaging also requires Visual Studio Installer Projects.

## Repairs and safety

- Simple, Standard and Extended modes use SFC, DISM, CHKDSK, Defender and drive
  optimization. The Windows system drive is detected instead of assuming `C:`.
- Cleanup is optional and asks for confirmation. Standard/Extended cleanup can
  permanently empty the Recycle Bin on **all drives** using the MSI-configured
  `cleanmgr` profiles (151/152/153). Without those profiles, cleanup depends on the
  machine's existing configuration; a CMake build does not import `cleanmgr.reg`.
- Full surface scans and offline disk repair require separate confirmation.
  Back up important data first. Offline CHKDSK uses its own console prompt; a
  reboot check is scheduled only when Windows confirms it. Scheduling is never
  inferred solely from the process exit code.
- If Windows servicing is pending, the user can restart or continue with DISM
  steps skipped. This check uses `WinSxS/pending.xml`, not every possible Windows
  reboot indicator.
- Each step reports failures/exit codes; failed silent steps show the final 8 KiB
  of output. Ctrl+C requests stopping after the active step, without forcibly
  terminating the repair process.
- Append `+` or `-` to a mode to override verbose output (`1+`, `2-`). Enter `+` or
  `-` alone to change the default. Invalid trailing input is rejected.
- Programs use absolute executable paths and no command shell. Language DLLs
  are loaded from the installation's `locale` directory **as resources**, without
  executing DLL initialization code. Missing translations fall back to English.

The batch scripts are shortcuts to the same `WRT.exe` implementation; they are
no longer independent copies of the repair commands. They require installed
binaries and the same administrator permissions and confirmations.

## Updates

The launcher checks GitHub over HTTPS with timeouts, bounded downloads and HTTP
status checking. It selects an unambiguous MSI asset from this repository's
release, validates its exact byte count and checks Authenticode trust. **Both the
launcher and MSI must be signed with the same trusted certificate.** Unsigned
development builds still run repairs, but cannot automatically install downloads;
use [manual releases](https://github.com/Layviz/WindowsRepairTool/releases).
Certificate rotation currently requires a manual update.

Downloaded installers use a unique, administrator/SYSTEM-only directory under
ProgramData. Only that download is cleaned up, never unrelated MSI files.
Updates run `msiexec /i` without a separate uninstall, keeping Windows Installer's
upgrade/rollback handling. Release maintainers must retain the MSI UpgradeCode
and generate appropriate ProductCode/PackageCode values for new versions. Signing
and MSI release packaging are not performed by the test workflow.

## Build and test

```powershell
cmake -S . -B build -A x64
cmake --build build --config Release --parallel
ctest --test-dir build -C Release --output-on-failure
```

The CMake output layout is `build/package/Release/{Binaries,locale,Batch scripts}`.
The Visual Studio solution's output layout is unchanged.

Portable unit tests also run on Linux, including address/undefined-behavior checks:

```sh
cmake -S . -B build -DCMAKE_BUILD_TYPE=Debug -DWRT_SANITIZERS=ON
cmake --build build --parallel
ctest --test-dir build --output-on-failure
```

Tests cover input, verbosity, narrow consoles, locale mapping, Windows argument
quoting, version parsing, bounded/chunked output, UTF-16 decoding, repair profiles,
exit-code interpretation, asset validation and failed/partial update transfers.
Windows tests launch only harmless fixture executables/DLLs to check binary output,
pipe draining, EOF, process errors, quoting and DLL/CWD isolation. **Tests never run
repairs, download releases, invoke MSI or reboot the machine.** GitHub Actions builds
and tests Windows Debug/Release plus Linux sanitizers.

### Nix (Linux)

The flake supports x86_64 and aarch64 Linux hosts. Its locked nixpkgs input
provides the compiler, CMake and Ninja without machine-specific store paths.

```sh
nix develop
cmake -S . -B build -G Ninja -DCMAKE_BUILD_TYPE=Debug -DWRT_SANITIZERS=ON
cmake --build build --parallel
ctest --test-dir build --output-on-failure
```

Cross-compile the Windows x64 application, German locale and batch shortcuts:

```sh
nix build .#windows  # also the default: nix build
```

`result/` contains `Binaries/`, `locale/` and `Batch scripts/`, including the
MinGW runtime DLLs next to the executables. Copy the entire directory to Windows;
this does not create an MSI, configure cleanup profiles or sign the executables.
MSI packaging still requires Visual Studio Installer Projects.

For an interactive MinGW environment, use a separate CMake build directory:

```sh
nix develop .#windows
cmake -S . -B build-windows -G Ninja -DCMAKE_SYSTEM_NAME=Windows \
  -DCMAKE_CXX_COMPILER="$CXX" -DCMAKE_RC_COMPILER=x86_64-w64-mingw32-windres \
  -DCMAKE_BUILD_TYPE=Release -DBUILD_TESTING=OFF
cmake --build build-windows --parallel
```

The interactive build writes to `build-windows/package/Release/`; unlike
`nix build`, it does not copy the runtime DLLs into that directory.
`nix flake check` runs the portable Release and ASan/UBSan suites and builds the
Windows package without executing it. Leak detection alone is disabled inside
the sandbox; no repairs, installers or reboots are run.

## Deutsch

Das Windows Reparatur Tool führt Windows-Bordmittel aus. Bereinigung,
vollständige Datenträgerprüfung und Offline-Reparatur müssen separat bestätigt
werden; die Bereinigung kann den Papierkorb aller Laufwerke unwiderruflich leeren.
Vor Datenträgerreparaturen wichtige Daten sichern. Fehler und Exitcodes werden
angezeigt, und das Windows-Systemlaufwerk wird automatisch erkannt.

Die Batchdateien starten dieselbe Anwendung statt eigene Befehlslisten.
Automatische Updates benötigen einen vertrauenswürdig signierten Launcher und
Installer mit demselben Zertifikat; unsignierte Builds werden manuell aktualisiert.
Build- und Testbefehle stehen oben. Unit-Tests und Windows-Testprogramme führen
keine echten Reparaturen, Downloads, Installationen oder Neustarts aus.
