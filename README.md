# TCPChat Qt — C++20 client and server

A cross-platform TCP chat project with **two separate C++ executables**:

- `tcpchat_server.exe` — headless server; run it on a Windows PC or Linux VPS.
- `tcpchat_client.exe` — graphical Qt Widgets application.

Both use C++20 and Qt 6 (Core, Network, Widgets). The client and server communicate using newline-delimited JSON over TCP. No Python runtime is needed by the application.

## Features

- Separate server and graphical client applications
- Dark Qt desktop UI
- Multiple simultaneous clients
- Nicknames, chat rooms, room user list, and timestamps
- Server/client address and port configuration
- JSON line framing and input size limits
- CMake build and a small integration test

## Build on Windows

### 1. Install the build tools

Install:

1. **Qt 6** using the official Qt Online Installer: <https://www.qt.io/download-qt-installer-oss>
   - Select Qt 6 and a desktop kit that matches your compiler, for example `MSVC 2022 64-bit`.
   - Install Qt Creator too if you want to build from its GUI.
2. **Visual Studio 2022 Build Tools** with the “Desktop development with C++” workload if using an MSVC Qt kit: <https://visualstudio.microsoft.com/downloads/>
3. **CMake** and Python 3 (Python is only needed to run the optional integration test).

The Qt kit and compiler must match. A MinGW Qt kit requires the matching MinGW compiler; an MSVC Qt kit requires MSVC.

### 2. Configure and compile

Open **Developer PowerShell for VS 2022** (or a terminal where the MSVC compiler is available), then change to this project directory. Adjust the Qt path to the version and kit you installed:

```powershell
cmake -S . -B build -DCMAKE_PREFIX_PATH="C:/Qt/6.8.3/msvc2022_64" -A x64
cmake --build build --config Release --parallel
```

For example, if you installed another Qt version, replace `6.8.3` and `msvc2022_64` with your actual folder names. You can also open the folder in Qt Creator, configure a Desktop kit, and build both targets there.

The executables will usually be at:

```text
build\Release\tcpchat_server.exe
build\Release\tcpchat_client.exe
```

With a single-configuration generator such as Ninja, they are usually directly in `build\` instead.

## Run locally on Windows

1. Start `tcpchat_server.exe` in a terminal. By default it listens on TCP port `5050`.
2. Start `tcpchat_client.exe` and connect to `127.0.0.1`, port `5050`.
3. Launch the client executable a second time and use another nickname to test multi-user chat.

The server accepts an optional port:

```powershell
.\build\Release\tcpchat_server.exe --port 5050
```

## Make a portable Windows app folder

Qt applications built with shared Qt libraries need Qt DLLs and the platform plugin alongside the executable. After building, run Qt's `windeployqt` from the same Qt kit. Example (adjust paths to your installation):

```powershell
C:\Qt\6.8.3\msvc2022_64\bin\windeployqt.exe .\build\Release\tcpchat_client.exe
C:\Qt\6.8.3\msvc2022_64\bin\windeployqt.exe .\build\Release\tcpchat_server.exe
```

Keep each executable with the files deployed next to it. You can then zip each app folder or create an installer. A truly single-file EXE with no Qt DLLs requires a static Qt build and additional licensing/build considerations; the normal Qt deployment is an EXE plus its required runtime files.

## Run server on Linux

Install Qt 6 development packages and CMake, then build using CMake. The server target only needs Qt Core and Network at runtime; the client additionally needs Qt Widgets and a desktop environment.

```bash
cmake -S . -B build -DCMAKE_PREFIX_PATH=/path/to/Qt/6
cmake --build build --parallel
./build/tcpchat_server --port 5050
```

For an internet-accessible server, configure the host firewall and cloud firewall to allow the chosen TCP port. Do not expose this prototype to an untrusted network without adding TLS, authentication, and abuse/rate controls. The current version is a learning MVP and does not encrypt messages.

## Integration test

With Python 3 installed, run:

```powershell
ctest --test-dir build -C Release --output-on-failure
```

The test launches the compiled server and checks connection, nickname handshake, message broadcast, and room switching.

## Protocol overview

Each TCP frame is a compact JSON object followed by `\n`. Client messages use `hello`, `message`, `join`, `nick`, and `who` types. The server sends `welcome`, `system`, `message`, `room`, and `users` events.

## License

MIT. See [LICENSE](LICENSE).

### Optional Windows helper scripts

From PowerShell, set `QT_DIR` to the installed kit folder, then build:

```powershell
$env:QT_DIR = "C:\Qt\6.8.3\msvc2022_64"
.\build_windows.bat
.\package_windows.bat
```

The package script creates separate `dist\client` and `dist\server` folders. Share the client folder with users; keep the server folder on the host machine. Use a Qt path matching your installed version and compiler kit.


## Automatic Windows build (EXE + Qt runtime files)

The GitHub Actions workflow in `.github/workflows/ci.yml` builds both Windows executables,
runs the integration test, deploys the required Qt/MSVC runtime files, and uploads two ZIP
artifacts: `tcpchat-client-windows` and `tcpchat-server-windows`.

To use it, push this project to a GitHub repository (or upload it to a repository you own),
open **Actions → Build and test TCPChat Qt**, and run the workflow (or trigger it with a push).
When it finishes successfully, open that workflow run and download the two artifacts from
the **Artifacts** section. The client ZIP is for each user's PC; the server ZIP goes on the
host machine. Keep the executable together with all files in its ZIP folder.
