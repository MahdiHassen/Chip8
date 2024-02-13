# Chip8 Interperter/Emulator with C and SDL2

Originally wrote this over the summer, decided to rewrite it recently to put it on Github. Rewriting this actually gives me some motivation to begin my Gameboy emulator that I've been putting off. 

## Brief Code Explaination
Anyway, all the source code is contained in the main.c file. It's ~700 lines of code, I suggest you download and open it in a code editor like VsCode so you're able to minimize functions, and switch cases that occupy a bunch of space. 

#### executeCycle():
The bulk of the code is in the executeCylcle() function, this is where I handle the various opcodes in one giant switch case. I don't think there's a better way to handle it. The explaination of each opcode is commented beside every specific case.

#### main
In the main function I handle inputs (I know, I know, I should do this somewhere else), and purposly delay our fetch exectue cycle so that we run at 500Hz/0.5Mhz. Chip 8 had no set time so I just chose 500 since that's what most people used. I do the same thing for the delay and sound timers which decrements at 60hz. You can load roms by including the path when you execute the file as the second argument, e.g:

❯ ./chip8.out "Tested Roms/3-corax+.ch8"

Otherwise it'll default to the IBM logo rom.

## Screenshots

Some screenshots of working roms, I can't test everything, but the ones in the folder have been tested:


<img width="1096" alt="Screenshot 2024-02-12 at 11 12 33 PM" src="https://github.com/MahdiHassen/Chip8-Interperter/assets/110603934/53486626-dcdb-4cd2-8e89-4a8c257efd99">
IBM Logo Rom (IBM logo.ch8)


<img width="1094" alt="Screenshot 2024-02-12 at 11 15 47 PM" src="https://github.com/MahdiHassen/Chip8-Interperter/assets/110603934/89260b87-cd47-4df4-8e87-ec0b6a58a944">
Corax Test Rom (3-corax+.ch8), used to test opcode implmentation


<img width="1094" alt="Screenshot 2024-02-12 at 11 14 54 PM" src="https://github.com/MahdiHassen/Chip8-Interperter/assets/110603934/bfad6a60-da09-4951-872a-2a226ca4ac41">
Pong (pong.rom)
