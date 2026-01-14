# Contributing to Maestro SPANK Plugin

Thank you for your interest in the Maestro SPANK Plugin!

## Contributions

At this time, **we do not accept external Pull Requests** for this repository. However, we highly value community feedback and are eager to improve the plugin based on your needs.

### How to Help

We encourage the following types of contributions via the **GitHub Issue tracker**:

1.  **Feature Requests**: If you have an idea for a new feature or an improvement to the current logic, please open a "Feature Request" issue. Provide a clear description of the use case and how it would benefit the integration.
2.  **Bug Reports**: If you encounter any issues, please submit a "Bug Report" with detailed steps to reproduce the problem and relevant environmental context.
3.  **Feedback**: General suggestions or questions about the plugin are also welcome.

## Local Development and Testing

If you would like to build or modify the code for your own local use, you can follow these steps:

### Prerequisites

- CMake (v3.10+)
- C++ Compiler (C++17)

### Building and Testing

```bash
mkdir build && cd build
cmake .. -DBUILD_TESTING=ON
cmake --build .
ctest --verbose
```

## Code Style

We use `.clang-format` to maintain a consistent code style. If you are working on the code locally, you can use the provided `.clang-format` file or set up the pre-commit hooks:

```bash
pip install pre-commit
pre-commit install
```
