# MassCode2MdFree

Standalone single-file C++17 CLI tool that exports a [MassCode](https://masscode.io) snippet database (`db.json`) into plain Markdown files.

## Output structure

Each snippet is written as a Markdown fenced code block under a folder that mirrors the MassCode folder hierarchy:

```
output_dir/
  FolderName/
    SnippetName.md
    MultiPartSnippet_1_from_2.md
    MultiPartSnippet_2_from_2.md
```

## Requirements

- C++17 compiler (GCC ≥ 7, Clang ≥ 5)
- [nlohmann/json](https://github.com/nlohmann/json) single-header library

## Building

### NixOS

```bash
nix-shell -p nlohmann_json gcc --run "g++ -std=c++17 -o mc2md mc2md.cpp"
```

Or enter a persistent shell using the provided `shell.nix`:

```bash
nix-shell
g++ -std=c++17 -o mc2md mc2md.cpp
```

### Debian / Ubuntu

```bash
sudo apt install nlohmann-json3-dev
g++ -std=c++17 -o mc2md mc2md.cpp
```

### Fedora / RHEL

```bash
sudo dnf install nlohmann-json-devel
g++ -std=c++17 -o mc2md mc2md.cpp
```

### Manual header download

```bash
mkdir -p nlohmann
curl -L https://github.com/nlohmann/json/releases/latest/download/json.hpp \
     -o nlohmann/json.hpp
g++ -std=c++17 -I. -o mc2md mc2md.cpp
```

## Usage

```
./mc2md -i <db.json> -o <output_dir>

Options:
  -i, --input   <file>   MassCode database file (db.json)
  -o, --output  <dir>    Output directory for Markdown files
  -h, --help             Show this help and exit
```

### Example

```bash
./mc2md -i ~/.config/massCode/db.json -o ~/snippets
```

## Notes

- Deleted snippets (marked `isDeleted: true`) are skipped.
- Fragment labels or values that are empty are skipped with a warning.
- Existing output files are never overwritten (skipped with a warning).
- Folder and snippet names are sanitised — only `[A-Za-z0-9_\-+]` is kept; everything else is replaced with `_`.
- The language identifier `c_cpp` is normalised to `cpp` in the fenced block.

## License

MIT © 2024–2025 Tomáš Mark
