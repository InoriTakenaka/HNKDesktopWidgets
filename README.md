# HNK Desktop Widgets

Native Windows desktop widget foundation. The management window is WinUI 3. Desktop widgets are ordinary Win32 windows rendered with Direct2D and DirectWrite. This repository is the initial architecture scaffold, not a finished widget platform.

## Technology stack

| Technology | Role |
|---|---|
| C++20 | Application core and native components |
| WinUI 3 / C++/WinRT | Management window, settings, and widget editor shell |
| Win32 | Widget HWND, input, monitors, and DPI |
| Direct3D 11 | GPU device |
| DXGI | Composition swap chain |
| Direct2D | Widget drawing |
| DirectWrite | Widget text |
| DirectComposition | Presents the swap chain on the widget HWND |
| MSBuild | Only build system |
| Windows SDK 10.0.26100.0 | Native Windows APIs |
| Windows App SDK 2.6.5 | WinUI 3 and the unpackaged runtime bootstrap |

MSBuild is the only build system. The solution does not use CMake.

Verified toolchain:

- Visual Studio Community 2026, version 18.10.3
- MSVC toolset v145 (14.51)
- Windows SDK 10.0.26100.0
- Windows App SDK NuGet package 2.6.5
- C++/WinRT NuGet package 2.0.250303.1
- Windows App Runtime 2.6.5 (x64), already installed on the development machine

The Visual Studio "Windows App SDK C++ Templates" component is not installed. The application project references the Windows App SDK through NuGet, so that workload component is not required to build.

## Architecture

`HNKDesktopWidgets.Core` owns the application lifetime, widget registry, widget instances, and configuration. It creates the native host and asks the platform and rendering projects to do the Windows and graphics work. It does not reference WinUI.

`HNKDesktopWidgets.Platform` creates the widget HWND, enumerates monitors, places the window on the primary work area, and records pointer input.

`HNKDesktopWidgets.Rendering` creates the Direct3D 11 device, DXGI composition swap chain, Direct2D context, DirectWrite text formats, and DirectComposition visual.

`HNKDesktopWidgets.Widgets` contains the widget types. The clock updates from a Win32 timer and requests a redraw only when the displayed text changes. Weather updates when `SetSummary` receives new text. The system monitor stores a sampling interval and does not query CPU or GPU data. None of them run a frame loop.

`HNKDesktopWidgets.App` is the WinUI 3 management shell. It registers the built-in widgets and starts the native host. WinUI does not draw the desktop widget.

## Directory structure

```text
HNKDesktopWidgets/
├── HNKDesktopWidgets.sln
├── Directory.Build.props
├── Directory.Build.targets
├── nuget.config
├── src/
│   ├── HNKDesktopWidgets.App/
│   ├── HNKDesktopWidgets.Core/
│   ├── HNKDesktopWidgets.Platform/
│   ├── HNKDesktopWidgets.Rendering/
│   └── HNKDesktopWidgets.Widgets/
├── resources/
└── plugins/
```

`resources` and `plugins` are reserved directories. Plugin loading is not implemented.

Shared compiler settings live in `Directory.Build.props` and `Directory.Build.targets`: C++20, toolset v145, Windows SDK 10.0.26100.0, warning level 4, and a Visual Studio NuGet restore check for the application project.

## Open and build

Open `HNKDesktopWidgets.sln` in Visual Studio 2026. The startup project is `HNKDesktopWidgets.App`. Build configurations are Debug and Release for x64.

From a Developer PowerShell for Visual Studio, in this directory:

```powershell
msbuild HNKDesktopWidgets.sln /restore /m /p:Configuration=Debug /p:Platform=x64
msbuild HNKDesktopWidgets.sln /restore /m /p:Configuration=Release /p:Platform=x64
```

If `msbuild` is not on `PATH`, use:

```powershell
& "C:\Program Files\Microsoft Visual Studio\18\Community\MSBuild\Current\Bin\MSBuild.exe" HNKDesktopWidgets.sln /restore /m /p:Configuration=Debug /p:Platform=x64
```

Build output goes to `build\` under this directory (`D:\Repository\HNKWallPaper\HNKDesktopWidgets\build` in this tree). Debug and Release share that folder, so the last configuration built is the one on disk. Launch the application with:

```powershell
.\build\HNKDesktopWidgets.exe
```

While the app is running it writes `hnkdesktopwidgets-verify.log` beside the executable. The log records the widget HWND, graphics initialization, redraw count, and the current clock text.

## Project dependencies

`HNKDesktopWidgets.App` references the static libraries and links them.

- `HNKDesktopWidgets.Core` uses `HNKDesktopWidgets.Platform` and `HNKDesktopWidgets.Rendering`.
- `HNKDesktopWidgets.Widgets` uses `HNKDesktopWidgets.Core` and `HNKDesktopWidgets.Rendering`.
- `HNKDesktopWidgets.Platform` and `HNKDesktopWidgets.Rendering` do not reference the other projects.
- Only `HNKDesktopWidgets.App` references the Windows App SDK and C++/WinRT packages.

The application also links `user32.lib` and `gdi32.lib` because the WinUI desktop project does not add those libraries by default.

## Packaging and deployment

The application is unpackaged. `WindowsPackageType` is `None` and `AppxPackage` is `false`.

An unpackaged process is a normal Win32 executable. It can create additional top-level HWNDs for desktop widgets without an MSIX package identity. The tradeoff is that package identity features are unavailable, including package-manifest startup tasks and Store MSIX updates. The Windows App SDK bootstrapper loads the installed framework package at startup.

This build is framework-dependent, not self-contained. The target machine needs the Windows App SDK 2.6 runtime that matches the NuGet package (`Microsoft.WindowsAppRuntime.2` version 2.6.5) and the Visual C++ runtime used by the toolset. Self-contained deployment can be enabled later with `WindowsAppSDKSelfContained`, which copies the runtime beside the executable and increases the output size.

## Verification

Built and run on this machine on November 10, 2026. The desktop widget window was inspected on the real desktop, over the wallpaper and over other windows.

- Debug and Release `HNKDesktopWidgets.sln` builds succeeded for x64.
- With acrylic requested, `hnkdesktopwidgets-verify.log` reported `acrylic_supported=true`, `acrylic_active=true`, `d2d=ok`, `dwrite=ok`, `dcomp=ok`, and `game_loop=false`.
- With acrylic requested, the widget was a frosted glass panel. Desktop icons and a white window behind it stayed visible as a blur, and the clock text stayed sharp.
- Choosing "Use solid background" switched that same window to the opaque fallback. The management window then read "Acrylic: disabled, solid background." and the button changed to "Use acrylic background."

## Acrylic backdrop

Desktop widgets use one shared backdrop. A widget does not call DirectComposition or DWM itself.

`AcrylicSettings` is the global appearance: enabled, tint color, tint opacity, and an opaque fallback color. `WidgetConfiguration.acrylic` is optional. When it is empty, `ResolveAcrylic` uses the global setting. When it is set, that widget replaces the global setting. The current host has one native window, so it uses the first configured widget's resolved setting.

On Windows 11 build 22621 or later, with Settings > Personalization > Colors > Transparency effects turned on, the widget HWND stays borderless and uses `WS_EX_NOREDIRECTIONBITMAP`. A composition thread samples the desktop behind that window with `CreateHostBackdropBrush`, blurs the sample, and keeps the premultiplied swap chain above it. The renderer clears to transparent and draws a thin white tint, so the blurred desktop remains visible. Text and the demonstration mark are then drawn fully opaque. DWM's own system backdrop is left off, because that material hides the desktop; `DWMWA_USE_HOSTBACKDROPBRUSH` only supplies the backdrop texture the composition brush samples.

If the OS build is older, transparency effects are off, or the backdrop cannot be created, the same window clears to the opaque fallback color instead. Disabling acrylic also selects that solid color. Nothing in this path adds a per-frame render loop.

## Known limitations

- One native host window is created, and the default configuration loads only the clock.
- Weather has no data source. The system monitor does not sample CPU, memory, or GPU.
- The widget editor displays configuration text and does not edit widgets.
- There is no plugin loader, theme system, animation system, or installer.
- The project targets x64 only.
- Per-pixel click-through is not implemented. Acrylic blurs what is behind the window, but pointer input still hits the widget.
