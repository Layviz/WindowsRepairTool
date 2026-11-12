{
  description = "Windows Repair Tool development, tests and Windows x64 cross-build";

  inputs.nixpkgs.url = "github:NixOS/nixpkgs/nixos-unstable";

  outputs =
    { nixpkgs, ... }:
    let
      systems = [
        "x86_64-linux"
        "aarch64-linux"
      ];
      forAllSystems = nixpkgs.lib.genAttrs systems;
      perSystem = forAllSystems (
        system:
        let
          pkgs = import nixpkgs { inherit system; };
          cross = pkgs.pkgsCross.mingwW64;
          src = pkgs.lib.cleanSource ./.;
          version = "1.4.3";

          unitTests =
            sanitizers:
            pkgs.stdenv.mkDerivation {
              pname = "wrt-tests${pkgs.lib.optionalString sanitizers "-sanitizers"}";
              inherit src version;
              nativeBuildInputs = [
                pkgs.cmake
                pkgs.ninja
              ];
              cmakeBuildType = if sanitizers then "Debug" else "Release";
              cmakeFlags = [
                "-DBUILD_TESTING=ON"
                "-DWRT_SANITIZERS=${if sanitizers then "ON" else "OFF"}"
              ];
              doCheck = true;
              # LeakSanitizer needs ptrace, which is unavailable in Nix sandboxes.
              # AddressSanitizer and UndefinedBehaviorSanitizer remain enabled.
              ASAN_OPTIONS = pkgs.lib.optionalString sanitizers "detect_leaks=0";
              UBSAN_OPTIONS = pkgs.lib.optionalString sanitizers "halt_on_error=1:print_stacktrace=1";
              checkPhase = ''
                runHook preCheck
                ctest --output-on-failure
                runHook postCheck
              '';
              installPhase = ''
                runHook preInstall
                install -Dm755 wrt_tests "$out/bin/wrt-tests"
                runHook postInstall
              '';
            };

          windows = cross.stdenv.mkDerivation {
            pname = "windows-repair-tool";
            inherit src version;
            strictDeps = true;
            nativeBuildInputs = [
              pkgs.cmake
              pkgs.ninja
            ];
            buildInputs = [ cross.windows.mcfgthreads ];
            cmakeFlags = [
              "-DCMAKE_SYSTEM_NAME=Windows"
              "-DCMAKE_RC_COMPILER=${cross.stdenv.cc.targetPrefix}windres"
              "-DBUILD_TESTING=OFF"
            ];
            # Windows binaries cannot run on the build host. Native unit tests
            # are separate checks; no repair programs are ever run here.
            doCheck = false;
            installPhase = ''
              runHook preInstall
              mkdir -p "$out"
              cp -r package/Release/. "$out/"
              for dll in libgcc_s_seh-1.dll libstdc++-6.dll; do
                cp "${cross.stdenv.cc.cc.lib}/${cross.stdenv.hostPlatform.config}/lib/$dll" "$out/Binaries/"
              done
              cp "${pkgs.lib.getBin cross.windows.mcfgthreads}/bin/libmcfgthread-2.dll" "$out/Binaries/"
              runHook postInstall
            '';
            # PE files and Windows resource DLLs must not be treated as native
            # Linux executables during the standard fixup phase.
            dontPatchELF = true;
          };
        in
        {
          packages = {
            inherit windows;
            default = windows;
          };
          checks = {
            unit = unitTests false;
            sanitizers = unitTests true;
            windows-build = windows;
          };
          devShells = {
            default = pkgs.mkShell {
              packages = [
                pkgs.cmake
                pkgs.ninja
              ];
            };
            windows = cross.mkShell {
              inputsFrom = [ windows ];
            };
          };
          formatter = pkgs.nixfmt;
        }
      );
    in
    {
      packages = forAllSystems (system: perSystem.${system}.packages);
      checks = forAllSystems (system: perSystem.${system}.checks);
      devShells = forAllSystems (system: perSystem.${system}.devShells);
      formatter = forAllSystems (system: perSystem.${system}.formatter);
    };
}
