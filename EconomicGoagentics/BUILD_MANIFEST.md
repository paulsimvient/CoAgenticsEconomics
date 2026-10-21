# CoAgentics Economics — v16 integrated source tree

This tree is assembled from the available Advanced Build iteration packages through Section 16.

## Assembly order

1, 2, 3, 4, 6, 7, 8, 10, 12.5, 13, 14, 15, 16 — later iterations overwrite conflicting files.

Sections 5, 9, and 11 did not have separate ZIP artifacts available in the Library; their changes are carried forward where they are present in later cumulative packages.

## Run

```bash
cd ~/Documents/code/CoAgentics
cmake -S . -B build
cmake --build build -j
ctest --test-dir build --output-on-failure
```

For this assembled tree, use its absolute path first:

```bash
cd /path/to/CoAgentics-Economics-v16
cmake -S . -B build
cmake --build build -j
ctest --test-dir build --output-on-failure
```

## Section 16

Section 16 adds the persistent/longitudinal memory subsystem (`PersistentMemory`) with explicit `None`, `Rolling`, and `Episodic` modes. Memory is an experimental factor; it should be recorded in campaign configuration and not treated as hidden model state.
