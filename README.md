# FileMaster3_modular

Menu-driven Linux file manager in C.

## Build

```bash
cd ~/FileMaster3_modular
make clean
make
./filemaster
```

The main menu accepts only integer choices 1 through 17. Any letters, symbols, mixed text, or numbers outside 1-17 show `Invalid choice` and ask again.

The program starts inside `workspace` and directory navigation is restricted to the workspace tree.
