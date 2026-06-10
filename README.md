# C++ backend (powered by drogon) for the *Vote* application

# GitHub Action workflows

The repository provides also two GitHub workflows to build the project on [GitHub runners](https://github.com/actions/runner). Both builds and tests the project using `vcpkg` and `CMake`, the only key difference is their implementation:

 - [hosted-pure-workflow.yml](.github/workflows/hosted-pure-workflow.yml): it is a __pure__ workflow which does not use unneeded GitHub Actions that cannot run locally on your development machine. On the other hand it is directly using the `CMake`, `Ninja`, `vcpkg` and the `C++ build` tools.
-  [hosted-ninja-vcpkg_submod.yml](.github/workflows/hosted-ninja-vcpkg_submod.yml): it is a concise workflow based on the custom GitHub Actions [get-cmake](https://github.com/lukka/get-cmake), [run-vcpkg](https://github.com/lukka/run-vcpkg) and [run-cmake](https://github.com/lukka/run-cmake) which simplify and shorten the workflow verbosity while adding some goodies like vcpkg binary caching stored on GH's cache and inline error annotations.


# License

All the content in this repository is licensed under the [MIT License](LICENSE.txt).
