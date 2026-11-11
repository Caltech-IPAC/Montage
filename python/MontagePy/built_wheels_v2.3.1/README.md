### Historical MontagePy 2.3.1 wheels

These binaries are retained for historical reference and should not be
used for current installations. They predate the image-buffer,
argument-order, and mViewer font-import fixes.

The CPython 3.11 macOS arm64 wheel has been seen to crash in
`mMakeImg`. The crash depends on what an uncleared buffer holds, so it
does not happen on every call. The fix is commit
[972ee04](https://github.com/Caltech-IPAC/Montage/commit/972ee04888f6a005e2705a97b7527d37b9587934).

See the [main MontagePy README](../README.md) for GitHub release
installation instructions and rebuilt wheels when available.
