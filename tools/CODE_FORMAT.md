# Coding style

## 1. Coding style

In order to make the code more readable, we recommend the following coding style:
[LLVM Coding Standards](https://llvm.org/docs/CodingStandards.html)

## 2. Wrapper formatting tool

For the convenience of formatting the code in the project, a wrapper script to format the code is provided. The script is located at `tools/code_formatting.py`, and it's based on `clang-format`.

Before running the script, the ``clang-format` should be installed. On Ubuntu, you can install it by running, or follow the instructions from the official documentation [llvm](https://apt.llvm.org/):

```bash
sudo apt-get install clang-format
```

Then run the script to format the specified files:

```bash
python3 tools/code_formatting.py --format-style=.clang-format --mode=0 src/iperf_server_api.c
```

## 3. IDE integration

IDE integration is a convenient way to format your code automatically when files are saved. You can follow the instructions from the official documentation [IDE integration]([tools/vim/README.md](https://clang.llvm.org/docs/ClangFormat.html#vim-integration)) to set up the integration.

## 4. Github CI integration

To ensure the code format is consistent, the code format check is added to the CI. Any illegal code format will cause the CI to fail.

# Context

* Version of iperf3: Any

* Hardware: Any

* Operating system (and distribution, if any): Any


* Other relevant information (for example, non-default compilers,
  libraries, cross-compiling, etc.):


# Bug Report

In order to make the code more readable, I suggest we can follow one of the coding styles:

- LLVM
- GNU
- Google
- Chromium
- Microsoft
- Mozilla
- WebKit

Maybe `LLVM` is more suitable for this project.

* Expected Behavior

All developers should have a consistent coding style.

* Actual Behavior

The code is not formatted consistently. For example, some files use tabs for indentation, while others use spaces.

* Steps to Reproduce

* Possible Solution

# Enhancement Request

* Current behavior

* Desired behavior

* Implementation notes

