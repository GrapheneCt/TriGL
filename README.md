# TriGL
WebGL implementation for SCE Trilithium Javascript engine.
Develop Javascript WebGL applications for PS Vita.

# Contents
### Plugin
1. liext.suprx - extends some features of Trilithium and adds simple TypedArray implementation. Currently provides:
- [Functions](https://github.com/GrapheneCt/TriGL/blob/main/plugin/liext/liext/functions.c)
- [Constants](https://github.com/GrapheneCt/TriGL/blob/main/plugin/liext/liext/constants.c)
2. webgl.suprx - provides full subset of WebGL functions. Extensions are not supported in the current version. Currently provides:
- [Functions](https://github.com/GrapheneCt/TriGL/blob/main/plugin/webgl/webgl/functions.c)
- [Constants](https://github.com/GrapheneCt/TriGL/blob/main/plugin/webgl/webgl/constants.c)
### Replacement
Replacement modules to disable builtin Trilithium renderer
### Samples
Various usage samples

# Installation
### Prepackaged
1. Download and install TriGL.vpk
### Manual
1. Download Crunchyroll app from PS Store
2. Decrypt all modules of the application with [FAGDec](https://github.com/CelesteBlue-dev/PSVita-RE-tools/tree/master/FAGDec/build) to .elf
3. Make fselfs from decrypted .elf modules with eny fself creation tools, for example from vitasdk
4. Replace `psp2/prx/graphics.suprx` and `psp2/prx/fwDialog.suprx` modules with replacement ones from replacement.zip
5. Copy [PVR_PSP2](https://github.com/GrapheneCt/PVR_PSP2/releases) modules to module folder
6. Copy modules from plugin.zip to `psp2/prx/`
7. Repackage app as .vpk

# Usage
1. Place your code to `js/app/`. Entry point file is main.js
2. Refer to samples for extention loading
