# particle life c
a (very basic) clone of [particle life](https://github.com/tom-mohr/particle-life-app) built with c and ~~sdl3~~ raylib :3

| <img src="https://raw.githubusercontent.com/personion222/particle-life-c/refs/heads/main/assets/trial1.gif"> | <img src="https://raw.githubusercontent.com/personion222/particle-life-c/refs/heads/main/assets/trial2.gif"> |
| :---: | :---: |
| <img src="https://raw.githubusercontent.com/personion222/particle-life-c/refs/heads/main/assets/trial3.gif"> | <img src="https://raw.githubusercontent.com/personion222/particle-life-c/refs/heads/main/assets/trial4.gif"> |

## features
- simple graphics through [raylib](https://www.raylib.com/)
- efficient, chunk-based particle interaction
	+ simplifies time complexity from O(n^2) to O(n)
	+ particles stored in linked lists for fast movement between chunks
- randomized particle attraction matrix
- blazingly fast 🚀 multi-threaded execution powered by [OpenMP](https://www.openmp.org/) (cpu utilisation go brrr)
- configurable through DRY macros at compile time
- randomize attraction matrix on `r` keypress

## development roadmap
- edge wrapping
- GUI and improved HUD
	+ live configuration without rebuilding project
	+ requires memory reallocation
- GPU (hardware accelerated) processing
- implement spatial hash grid for unlimited map size
- navigation tools
	+ zoom, pan, screenshot
- eye candy
	+ bloom around particles
	+ render circles rather than pixels

## usage
⚠️ untested on windows/macOS! theoretically should work though
1. ensure you have [raylib](https://www.raylib.com/), [gcc](https://gcc.gnu.org/), and [make](https://www.gnu.org/software/make/) installed for your system
2. clone the repository to your device
3. configure macros at the beginning of main.c to your liking
4. run `make` and `./main.out` to launch
	+ if debugging with gdb, build with `make debug` instead