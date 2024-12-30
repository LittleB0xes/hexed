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
This project use [Termbox2](https://github.com/termbox/termbox2)

## Usage
You can work on one file
> ./rhexed my_file

or, if you need, on several files in the same time, with the ability to navigate from file to file.
> ./rhexed my_file_1 my_file_2 my_file_3 ...

## Command
```
- hjkl      move (arrows works too)
- g         move to the beginning of the file
- G         move to the end of the file
- (         move to the beginning of the line
- )         move to the end of the line
- [         move to the beginning of the page
- ]         move to the end of the page
- n         next page
- b         previous page
- N         next file
- P         previous file
- J         jump to address
- a         insert a byte after cursor position
- x         cut a byte
- y         copy a byte 
- p         paste a byte
- i         insert mode
- I         insert mode (in ascii) -- but not accented or other local stuffs :-(
- <ESC>     quit insert mode
- <SPACE>   force refresh
- <TAB>     show / hide title
- w         write file
- q         quit
```

