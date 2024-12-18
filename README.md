# HexeD - A work in progress Hex editor

This is a work in progress hex editor made in C, for the fun. Features will be added over time.


## Use
```
./hexed my_file
```

## Commands

- h,j,k,l : move
- g : go to the beginning of the file
- G : go to the end of the file
- 0 : go to the beginning of the line
- $ : go to the end of the line
- x : cut a byte at the cursor position
- y : copy the byte at the cursor position
- p : paste a byte after the cursor position
- a : add 0x00 byte at the cursor position
- i : switch to insert mode
- <ESC> : quit insert mode
- q : quit without saving
- w : save the file
