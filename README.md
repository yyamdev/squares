# Squares
A sidescrolling rhythm game written in C and Javascript.
Play the game on [itch.io](https://yyam.itch.io/squares).

Squares compiles to WebAssembly using Clang. It does not require Emscripten or the C standard library.

# Building
Build scripts are provided for Windows and Linux that use Clang to build a WASM module.
* The scripts expect a directory called "assets" to be present in the root directory of the repository. This directory is not tracked in this repository but can be downloaded [here](https://github.com/yyamdev/squares/releases/tag/v1.0.0). (Hosted on GitHub as a release.)
* Open a terminal/command prompt.
* Navigate to the root directory of the repository.
* Ensure Clang available in the current session.
* Run `tools/build.bat` or `tools/build.sh`

# Running
* Open `build/index.html`
* Depending on your browser, you may have to access index.html with the `http` protocol (instead of `file:///`). This may require running a minimal web server on your machine.