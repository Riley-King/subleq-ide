# SUBLEQ IDE
### A terminal based editor for a binary SUBLEQ PC.

### Features
 - Conditional and regular breakpoints
 - Execute a single instruction at a time (step)
 - Execute until told to stop or breakpoint
 - Reset program (only for loaded files)
 - Edit values
 - Set instruction pointer, which is also saved in the binary file
 - Uses nano-style keybinds, just without ctrl/alt

### Planned Features
 - Improve 'rendering' code to only redraw what changes to minimize flicker.
 - Support for linux
 - Support for non-x86_64 platforms
 - Move away from using visual studio project files to a bash/batch script for building
 - loading and assembling a high-level assembley language
 - 'run until' - use breakpoints for now
 - 'step x times' - use breakpoints and single steps for now
 - Store arbitary 'saves' in memory to allow different points to reset to
 - A built-in assembley editor for previously mentioned high-level assembley
