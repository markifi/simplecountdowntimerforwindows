# simplecountdowntimerforwindows
copying hourglass (https://chris.dziemborowicz.com/apps/hourglass/) except not .NET. a simple countdown timer for windows. much worse than hourglass, but starts instantly.

- config file can be opened by pressing e
- default font is atkinson hyperlegible next mono light (https://github.com/googlefonts/atkinson-hyperlegible-next-mono/raw/refs/heads/main/fonts/ttf/AtkinsonHyperlegibleMono-Light.ttf)
- g++ -O3 -std=c++17 simplecountdowntimerforwindows.cpp -o simplecountdowntimerforwindows.exe -luser32 -lgdi32 -lwinmm -lshell32 -mwindows -ucrt
- https://winlibs.com/#download-release
- C:\mingw64\bin
