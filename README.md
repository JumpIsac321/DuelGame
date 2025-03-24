run: 
git clone https://github.com/libsdl-org/SDL.git vendored/SDL
cmake -S . -B build
cmake --build build

and the executable will be in build/
