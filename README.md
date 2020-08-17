# MAXE

A message driven simulator for agent-based models, primarily developed for the simulation of various financial markets. Features, among other things
*  simulation of LOBs with customizable matching algorithm (price-time, pure pro-rata, priority pro-rata, or priority pro-rata)
*  L1, by-order and by-trade logging agents (providing both human-readable and CSV output)
*  Bouchaud's zero-intelligence agent
*  an agent for impact trading
*  a generic interface for design of custom agents

```
Usage: maxe [OPTIONS] [file] [params...]
  file      the simulation file to be used (default: Simulation.xml)
  params    Parameters to be passed to the simulation configuration & the
            simulation itself

Options:
  -f, --file=STRING          the simulation file to be used (default:
                             Simulation.xml)
  -i, --interactive / --no-interactive
                             runs the simulation in the interactive mode
  -r, --runs=NUM             Number of times the simulation is to be run
                             (default: 1)
  -s, --silent / --no-silent  supresses all verbose trace output, error traces
                             remain enabled

  --help                     Show this message and exit.
```

## Installation
You can build MAXE using the CMake configuration it comes with (CMake 3.15+ required).

On Windows you need
* `cmake` 3.15+
* MSVC 15.7+
* `python` on your system (tick the Python C headers options in the installation if not automatically included in your python version)

On Linux (tested with Ubuntu) you need
* `cmake` 3.15+
* GCC 9.0+ (similarly slave `g++` of the same version, or manually add `filesystem` dependency, i.e. `#include <experimental/filesystem>` and compile with `-lstdc++fs`), *or* LLVM Clang 9.0+ (or LLVM 7.0+ and LLVM 8.0+ with the aforementioned experimental and `stdc++fs` fix)
* `python` on your system (preferably with `python-all` or at least `distutils`)
* `python-dev` for Python C headers

Remember that if you're using Python 3, you will need the `python3` equivalents of everything (i.e. `python3-all`, `python3-dev`).

On MacOS you need
* MacOS 10.15 Catalina or newer (for `std::filesystem::path` support)
* Xcode 11.0+ (for  `#include <filesystem>` to work, otherwise see the note for LLVM 9.0 above)
* LLVM Clang 9.0+

To build, say that you already cloned (with `--recurse-submodules`! Otherwise do `git submodule init && git submodule update`) the repository into a directory called `thesimulator` (`git` default). Then run the following

```
cd thesimulator

mkdir build
cd build
cmake ../
cmake --build .
```

MAXE can then be run by executing the `TheSimulator` executable. Alternatively, CMake GUI can be used on all platforms to configure and generate makefiles (or equivalent on Windows) and then `make`.

## Embedding a Python Script
It's pretty straightforward. See [the official Python documentation on the topic](https://docs.python.org/3/extending/embedding.html) for more info.

For `pybind11`, one needs the pointer width of the Python installation match the pointer width of the compilation output
