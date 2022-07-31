# RF5ModLoader
 A Rune Factory 5 (RF5) Mod Loader (Windows) 
# Support
-  BepInEx (Windows)
# BepInEx Installation
- Extract the contents of the download and copy the contents to the directory ``BepInEx/plugins``
# Loading Mods
- In the directory containing the game executable, the directory ``mods`` is to load game files to patch at runtime
  - ex) ``mods\StreamingAssets\aa\StandaloneWindows64\382aa2501e16ef256258fc102a8fcea5.bundle`` to patch the file in ``Rune Factory 5_Data\StreamingAssets\aa\StandaloneWindows64\382aa2501e16ef256258fc102a8fcea5.bundle``
# Building
## RF5ModLoader
### Dependencies
- [polyhook2](https://github.com/stevemk14ebr/PolyHook_2_0)

Install the aforementioned dependency with your preferred package manager, preferrably ``vcpkg``, or build manually.

To install with vcpkg, run the command ``vcpkg install polyhook2:x64-windows-static``.
