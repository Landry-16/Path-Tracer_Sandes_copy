# Display, Interactive Mode, and Tools

This document covers the live SFML display, the interactive render loop,
hot-reload via `FileWatcher`, the standalone `denoise_tool` binary, and the
command-line interface.

---

## Table of contents

1. [ProgressiveDisplay](#1-progressivedisplay)
2. [InteractiveMode](#2-interactivemode)
3. [FileWatcher](#3-filewatcher)
4. [denoise\_tool](#4-denoise_tool)
5. [Command-line interface](#5-command-line-interface)
6. [Startup flow in main](#6-startup-flow-in-main)

---

## 1. ProgressiveDisplay

**Source:** `src/display/ProgressiveDisplay.cpp`  
**Header:** `include/rt/display/ProgressiveDisplay.hpp`

`ProgressiveDisplay` wraps an SFML window and exposes a callback-based API so
that the render thread can update pixels without knowing anything about SFML.

### Window creation

```cpp
ProgressiveDisplay(int width, int height);
```

Creates an `sf::RenderWindow` with mode `sf::VideoMode(width, height)` and
title `"Ray Tracer - Progressive Render"`. The SFML event loop is capped at 30
calls per second.

### Pixel buffer

```cpp
std::vector<uint8_t> pixels_;   // RGBA, length = width * height * 4
std::mutex pixelMutex_;
std::atomic<bool> needsUpdate;
```

All pixel writes go through `updateRegion()`, which locks `pixelMutex_` before
modifying `pixels_` and then sets `needsUpdate`. All reads in `draw()` also
lock the same mutex.

### updateRegion

```cpp
void updateRegion(int x, int y, int w, int h,
                  const std::vector<Color> &colors);
```

Converts the `Color` (0.0-1.0 doubles) to 8-bit RGBA and writes them into the
flat pixel buffer at the correct offsets. The region is clamped to the window
bounds. Called from the render thread.

### draw

```cpp
void draw();
```

Called from the SFML thread (the main thread). If `needsUpdate` is true, locks
`pixelMutex_`, calls `texture_.update(pixels_.data())`, resets the flag, then
draws the sprite. A status text overlay (`sf::Text`) is rendered at screen
position (10, 10) in white with a black outline.

### eventLoop / run

```cpp
void run();       // blocks until the window is closed
void processEvents();
```

Polls SFML events and calls `draw()`. Sleeps 16 ms per iteration. `run()` is
called from the main thread; the render thread calls `updateRegion()` in
parallel.

### Callbacks

Set before calling `run()`:

```cpp
display->setSaveCallback(    [&]() { /* write PPM */    });
display->setReloadCallback(  [&]() { /* reload scene */ });
display->setCameraCallback(  [&](const std::string &action) { /* move camera */ });
```

### Key bindings

| Key | Action |
|---|---|
| Escape | Close window, set `stopRequested` to true |
| S | Invoke `saveCallback_` |
| R | Invoke `reloadCallback_` |
| Arrow Up / I | `cameraCallback_("forward")` |
| Arrow Down / K | `cameraCallback_("backward")` |
| Arrow Left / J | `cameraCallback_("yaw_left")` |
| Arrow Right / L | `cameraCallback_("yaw_right")` |
| U | `cameraCallback_("pitch_up")` |
| O | `cameraCallback_("pitch_down")` |
| Q | `cameraCallback_("left")` |
| E | `cameraCallback_("right")` |
| Space | `cameraCallback_("up")` |
| C | `cameraCallback_("down")` |

---

## 2. InteractiveMode

**Source:** `src/app/InteractiveMode.cpp`

`runInteractiveMode()` is the entry point for the live window mode. It runs
when `--no-display` is not passed on the command line.

### Thread architecture

The main thread owns the SFML window and blocks in `display->run()`. The render
thread does the heavy computation and pushes pixels into the display via
`updateRegion()`.

### InteractiveCtx

```cpp
struct InteractiveCtx {
    Scene *scene;
    RenderConfig *cfg;
    ProgressiveDisplay *display;
    FileWatcher *watcher;
    std::atomic<bool> stopRequested;
    std::atomic<bool> reloadRequested;
    std::atomic<bool> cameraChanged;
    std::mutex sceneMutex;
};
```

All shared state flows through `InteractiveCtx`. The atomics are written by
callbacks (main thread) and read by the render thread.

### runRenderLoop

```cpp
void runRenderLoop(InteractiveCtx &ctx);
```

Calls `runRenderFrame()`, then `waitForRenderTrigger()`, then
`processRenderTrigger()` in a loop until `stopRequested` is set.

### runRenderFrame

```cpp
void runRenderFrame(InteractiveCtx &ctx);
```

Locks `sceneMutex`, then calls either:
- `PathTracer::renderWithDisplay()` when `cfg.usePathTracing` is set, or
- `Renderer::renderMultithreadedWithDisplay()` otherwise.

If `cfg.useDenoise` is true it denoises after the render completes.

### waitForRenderTrigger

```cpp
void waitForRenderTrigger(InteractiveCtx &ctx);
```

Polls every 100 ms. Returns when any of these is true:
- `display->isStopRequested()` 
- `ctx.reloadRequested`
- `ctx.cameraChanged`
- `ctx.watcher->hasChanged()`  (file modified on disk)

### processRenderTrigger

```cpp
void processRenderTrigger(InteractiveCtx &ctx);
```

- If the scene file was modified on disk: reloads the scene from disk, resets
  `reloadRequested`, resets the `FileWatcher` timestamp.
- If only `cameraChanged` is set: does not reload, just re-renders with the
  updated camera.

### Camera control

Speed constants defined in `InteractiveMode.cpp`:

| Constant | Value |
|---|---|
| `moveSpeed` | 0.3 units/keypress |
| `rotateSpeed` | 5.0 degrees/keypress |

Actions handled by `setupInteractiveCallbacks()`:

```
forward / backward   -> translate camera along its forward vector
left / right         -> strafe
up / down            -> translate along world Y
pitch_up / pitch_down -> rotate around local X axis
yaw_left / yaw_right  -> rotate around world Y axis
```

Each action sets `ctx.cameraChanged = true`, triggering a re-render.

### Save callback

The save callback uses `PPMWriter` to write the current pixel buffer to the
path specified in `RenderConfig::outputFile`.

---

## 3. FileWatcher

**Source:** `src/core/FileWatcher.cpp`  
**Header:** `include/rt/core/FileWatcher.hpp`

`FileWatcher` detects whether a file has been modified since it was last
observed.

```cpp
FileWatcher(const std::string &filePath);
bool hasChanged();
void reset();
```

Implementation:

1. The constructor calls `stat()` on `filePath` and stores `st_mtime` as
   `lastModTime_`.
2. `hasChanged()` calls `stat()` again. Returns `true` if the new `st_mtime`
   differs from `lastModTime_`.
3. `reset()` updates `lastModTime_` to the current `st_mtime`, clearing the
   changed flag.

`InteractiveMode` calls `reset()` before entering the render loop, then calls
`hasChanged()` every 100 ms inside `waitForRenderTrigger()`. On detection it
reloads the scene and calls `reset()` again.

The `stat()` approach has no kernel file-descriptor overhead and works on all
POSIX systems. Granularity is one second on most Linux filesystems.

---

## 4. denoise\_tool

**Source:** `src/denoise_tool.cpp`

`denoise_tool` is a standalone binary separate from the main raytracer. It
reads a PPM image, denoises it, and writes the result to another PPM file.

### Usage

```
./denoise_tool <input.ppm> <output.ppm>
```

Both arguments are required. Exit code 0 on success, 1 on any error.

### Denoiser selection

`loadDenoiser()` tries to locate a denoiser plugin in this order:

1. **OIDN denoiser**: looks for a shared library at the path given by the
   environment variable `OIDN_DENOISER_LIB_PATH`. If the variable is unset it
   falls back to `./plugins/oidn_denoiser.so`. Uses C ABI:
   ```c
   IDenoiser *create_denoiser();
   void       destroy_denoiser(IDenoiser *);
   ```
2. **Simple denoiser**: falls back to `SIMPLE_DENOISER_LIB_PATH` or
   `./plugins/simple_denoiser.so`. Uses C ABI:
   ```c
   IDenoiser *create_simple_denoiser();
   void       destroy_simple_denoiser(IDenoiser *);
   ```

If neither library is found, the tool prints an error and exits with code 1.

### DenoiserOwner

```cpp
struct DenoiserOwner {
    IDenoiser *ptr;
    std::function<void(IDenoiser *)> destroyer;
    void *libHandle;
    ~DenoiserOwner() { destroyer(ptr); dlclose(libHandle); }
};
```

RAII wrapper that holds the raw denoiser pointer, its matching destroy
function, and the `dlopen` handle. The destructor ensures `destroy_denoiser()`
is called before `dlclose()`, preventing a use-after-free of vtable pointers.

### PPM loading

```cpp
std::vector<Color> loadPPM(const std::string &path, int &width, int &height);
```

Reads P3 (ASCII) PPM format:
1. Reads magic number `P3`.
2. Reads width and height.
3. Reads max value (typically 255).
4. Reads `R G B` triples and normalises them to 0.0-1.0 `Color` values.

The function throws `std::runtime_error` for malformed files.

---

## 5. Command-line interface

**Source:** `src/app/RenderConfig.cpp`

```
./raytracer <scene_file> [output_file] [options]
```

`<scene_file>` is the only required argument. It is always `argv[1]`.

### Options

| Flag | Type | Default | Description |
|---|---|---|---|
| `--path-tracing` | bool | false | Use the path tracer instead of the Whitted-style renderer |
| `--samples N` | int | 64 | Samples per pixel for the path tracer (ignored for Whitted) |
| `--denoise` | bool | false | Apply denoising after rendering (offline and interactive) |
| `--no-display` | bool | false | Render to file without opening an SFML window |
| `--single-thread` | bool | false | Use a single thread (useful for debugging or determinism) |
| `output_file` | string | `output.ppm` | Any non-flag argument after `<scene_file>` is treated as the output path |

`--samples N` silently clamps to a minimum of 1:
```cpp
cfg.samplesPerPixel = std::max(1, std::atoi(next_arg));
```

Unrecognised flags print a warning to stderr and are ignored.

### RenderConfig struct

```cpp
struct RenderConfig {
    std::string sceneFile;
    std::string outputFile    = "output.ppm";
    bool usePathTracing       = false;
    bool noDisplay            = false;
    bool singleThread         = false;
    bool useDenoise           = false;
    int  samplesPerPixel      = 64;
};
```

---

## 6. Startup flow in main

**Source:** `src/main.cpp`

```
main()
  |
  +--> ApplicationCleanup raii_guard   // registers atexit cleanup
  |
  +--> loadAndListPlugins()
  |      |
  |      +--> PluginManager::instance().loadPluginsFromDirectory("./plugins")
  |      +--> print registered primitive types
  |
  +--> parseArgs(argc, argv)           // fills RenderConfig
  |
  +--> Factory::loadScene(cfg.sceneFile)  // libconfig -> Scene
  |
  +--> if cfg.noDisplay:
  |      renderOffscreen(scene, cfg)
  |        +--> PathTracer or Renderer
  |        +--> optionally denoise
  |        +--> PPMWriter::write()
  |
  +--> else:
         runInteractiveMode(scene, cfg)
           +--> spawn render thread
           +--> display->run() blocks here
```

`ApplicationCleanup` destructor order (guaranteed on exit):

1. `Factory::clearAllRegistries()` -- invalidates all factory lambdas
2. `PluginManager::instance().unloadAll()` -- `dlclose()` all plugins
