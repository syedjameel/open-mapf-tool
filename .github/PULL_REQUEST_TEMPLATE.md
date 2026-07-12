# Summary

<!-- What does this PR change, and why? Link related issues. -->

# Checklist

- [ ] `ctest` passes locally (`cmake -B build -DOPENMAPF_WERROR=ON && cmake --build build && ctest --test-dir build`)
- [ ] New/changed logic is covered by tests (fixtures under `tests/fixtures/` where applicable)
- [ ] Tree is formatted (`clang-format` 18, see CONTRIBUTING.md)
- [ ] Docs updated if formats, CLI flags, or the results-CSV schema changed (schema is append-only)
- [ ] No vendored code (`vendor/`, `src/imfilebrowser.h`) was reformatted or modified
