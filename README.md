# WPS Installer Launcher

Build `Release|x64` or `Release|x86`, matching the target process architecture.

The output directory contains:

- `Launcher.exe`
- `Anchor.dll`

Run:

```text
Launcher.exe "C:\Full\Path\To\Target.exe"
```

The launcher creates the target process suspended, loads `Anchor.dll` from the launcher's own directory, then resumes the target main thread.
