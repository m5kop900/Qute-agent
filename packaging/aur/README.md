# AUR packaging

`qute-agent-git/PKGBUILD` builds the current `main` branch. It is a VCS package because the
project has no release tag yet (`PKGBUILD` explains how to switch to a stable tarball later).

## Build locally
```
cd packaging/aur/qute-agent-git
makepkg -sri          # build, install, and check dependencies
```

## Check before publishing
```
namcap PKGBUILD
makepkg --printsrcinfo > .SRCINFO
```

## Publish
1. Create an AUR account and SSH key: <https://aur.archlinux.org/>.
2. `git clone ssh://aur@aur.archlinux.org/qute-agent-git.git`.
3. Copy `PKGBUILD` and `.SRCINFO` into it, then `git add PKGBUILD .SRCINFO && git commit && git push`.
4. Keep `.SRCINFO` in sync: `makepkg --printsrcinfo > .SRCINFO` after every `PKGBUILD` change.

## Before the first upload
- **License:** the repository has no `LICENSE` file, so the PKGBUILD uses `license=('custom')`.
  Pick the real license, add the file, set `license=('...')`, and install the file in `package()`.
  AUR rejects packages with an unclear license.
- **Maintainer:** replace the placeholder `Maintainer:` line with your name and email.
- The install rules put both binaries in `/usr/bin`, sprite packs in `/usr/share/qute/spritepacks`,
  desktop entries + icons in the standard XDG folders, docs in `/usr/share/doc/qute`, and the GNOME
  Shell extension in `/usr/share/qute/gnome-shell` (users enable it with `install.sh`).
