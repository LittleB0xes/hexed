```
db   db d88888b db    db d88888b d8888b.
88   88 88'     `8b  d8' 88'     88  `8D
88ooo88 88ooooo  `8bd8'  88ooooo 88   88
88~~~88 88~~~~~  .dPYb.  88~~~~~ 88   88
88   88 88.     .8P  Y8. 88.     88  .8D
YP   YP Y88888P YP    YP Y88888P Y8888D'
```


# HexeD - a WIP hex file editor

At the same time, I have the same project but in [Rust](https://github.com/LittleB0xes/rhexed)

## Usage
```
./rhexed my_file
```

## Command
```
- hjkl        move 
- g           move to the beginning of the file
- G           move to the end of the file
- (           move to the beginning of the line
- )           move to the end of the line
- [           move to the beginning of the page
- ]           move to the end of the page
- n           next page
- b           previous page
- J           jump to address
- a           insert a byte after cursor position
- x           cut a byte
- y           copy a byte 
- p           paste a byte
- i           insert mode
- <ESC>       quit insert mode
- w           write file
- q           quit
```

