# Project
- I want to create a patch that changes the control scheme of the game. The original game uses tank controls. The patch will remove the tank controls and add modern 3D controls.
- Controls for the orignal game:
    - Up Arrow: Move forward
    - Down Arrow: Move backward
    - Left / Right Arrows: Turn left or right
    - A Key + Arrow Key: Move faster or run in the selected direction
    - Z Key + Left/Right Arrow: Sidestep left or right
    - Z Key + Up/Down Arrow: Shuffle slowly (prevents falling off any ledges) 
    - Tab Key: Open the main menu
    - Tab Key (Held Down) + Arrow Keys: Look around the environment
    - Caps Lock: Fire a ranged weapon while in freelook/first-person view
- Tank Controls: Pressing up moves forward, pressing down steps back, and pressing left or right rotates the character on the spot.
- Modern Controls: Something similar to Mario 3D. This plan still needs work. 


# Game Info
- GoG version comes with a pre-installed nGlide wrapper: 3DFx Glide wrapper. This emulates original 90s Voodoo graphics cards, making it highly stable on modern GPUs.
- The orignal PC game was released in North America on 25, June 1998.
- The game is software rendered.
- The game uses Direct3D 5.
- The Exe is 32-bits.

# Game Files
Deathtrap Dungone GoG game file directory:
- C:\GOG Games\Deathtrap Dungeon
    - This contains `Dungeon.DLL` - the game code
- C:\GOG Games\Deathtrap Dungeon\ASYLUM
    - This contains `DD_CD.EXE` - files used to launch the game


# Tools
Ghidra:
- C:\tools\ghidra_12.1.4_PUBLIC
Ghidra Deathtrap Dungeon project files:
- C:\dev\ghidra-projects\DD-REA (current, annotated)
- C:\dev\ghidra-projects\DD (old, pre-REA analysis)
REA results:
- C:\dev\rea-projects\DD\results
Mod code, Ghidra scripts and these docs:
- C:\dev\DD-mod
x32dbg:
- C:\tools\x64bdg_2026-05-27_12-11\release\x32