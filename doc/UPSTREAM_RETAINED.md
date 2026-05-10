# Upstream-only files retained

The ECMHack ALDL-IO 1.6.2 source distribution changed the source tree layout and no longer includes some files that are present in the original GitHub repository.

This fork intentionally keeps selected upstream-only files for reference and possible future use:

- `doc/README.developers`
- `modules/dataserver.c`
- `modules/dataserver.h`

The 1.6.2 tree still ships `config-examples/dataserver.conf`, so keeping the original dataserver module source is useful while the project is being audited and modernized.
