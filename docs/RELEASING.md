# Releasing OpenMAPF (maintainers)

Step-by-step from the `main` branch to an archived,
citable release. Steps marked **[manual]** need the maintainer's accounts
or a machine with a display.

## 1. Pre-flight

1. **Hexagonal LaCAM solvers.** The three solvers are public
   (`github.com/syedjameel/hexagonal-lacam`, `-lacam2`, `-lacam3`, tag
   `v1.0.0`) and archived on Zenodo (10.5281/zenodo.23475645, .23475834,
   .23476002). For a new solver release: tag the repositories, deposit the
   new version on Zenodo, and update the tag in `scripts/get-solvers.sh`
   and the URLs and checksums in the `Dockerfile`.
2. **[manual] Enable GitHub Discussions** (README and CONTRIBUTING point
   to them as the questions channel).

## 2. Push and verify CI

```sh
git push origin main
```

The first CI run happens now — it has never run remotely. Verify on the
Actions tab: the two Linux jobs, `format`, and `sanitize` must be green;
`macos-14` is best-effort (`continue-on-error`) — if it passes, remove the
`experimental: true` flag in `.github/workflows/ci.yml` and state macOS
support honestly in the README/metapaper; if it fails, file the log as an
issue and leave macOS unclaimed.

Open a PR to `main`, review, merge.

## 3. Version and tag hygiene

A **local-only** baseline tag `v0.1.0` points at the pre-hardening
prototype (`ca4b616`; it was never pushed). Re-point the name at the real
release:

```sh
git tag baseline-prototype ca4b616     # keep the baseline reachable, optional
git tag -d v0.1.0                      # remove the local baseline tag
```

Then, on the merge commit:

1. Set `date-released: "YYYY-MM-DD"` in `CITATION.cff` (keep the format
   quoted; re-validate with `cffconvert --validate`).
2. In `CHANGELOG.md`, change `## [0.1.0] — unreleased` to
   `## [0.1.0] — YYYY-MM-DD`.
3. Confirm `project(openmapf VERSION 0.1.0 ...)` in CMakeLists.txt matches.
4. Commit (`chore: release 0.1.0`), then:

```sh
git tag -a v0.1.0 -m "OpenMAPF 0.1.0"
git push origin main v0.1.0
```

(Repeat this section for each later release; each release gets its own
Zenodo version DOI.)

## 4. GitHub release with binaries

```sh
cmake -B build -DCMAKE_BUILD_TYPE=Release && cmake --build build --parallel
(cd build && cpack)     # produces openmapf-0.1.0-Linux.tar.gz and openmapf_0.1.0_amd64.deb
```

**[manual]** Create a GitHub release from the `v0.1.0` tag, paste the
CHANGELOG 0.1.0 section as the release notes, and attach both CPack
artifacts.

## 5. Zenodo DOI

1. **[manual] Before creating the release** (or re-publish the release
   afterwards): log in to zenodo.org with the GitHub account, enable the
   webhook for `syedjameel/open-mapf-tool`
   (Zenodo → Account → GitHub → toggle the repo on).
2. Publishing the GitHub release triggers the deposit; Zenodo mints a
   **version DOI** and a **concept DOI**. Check the metadata Zenodo
   derived (it reads CITATION.cff); fix authors/title on Zenodo if needed.
3. **Backfill the version DOI** (one commit, `chore: add Zenodo DOI`):
   - `CITATION.cff`: add `doi: 10.5281/zenodo.XXXXXXX` (replace the
     comment at the top of the file).
   - `README.md`: replace the DOI-badge placeholder comment with
     `[![DOI](https://zenodo.org/badge/DOI/10.5281/zenodo.XXXXXXX.svg)](https://doi.org/10.5281/zenodo.XXXXXXX)`.

## 6. Post-release checklist

- [ ] CI badge green on `main`
- [ ] `scripts/get-solvers.sh` works on a clean machine (real solver repos)
- [ ] `pip install pandas matplotlib && scripts/openmapf-plot ...` works on
      the release tarball
- [ ] CITATION.cff validates and shows the DOI
- [ ] Screenshots render on the GitHub front page
- [ ] Start the next CHANGELOG section: `## [Unreleased]`
