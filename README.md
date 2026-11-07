# simplecountdowntimerforwindows
copying hourglass (https://chris.dziemborowicz.com/apps/hourglass/) except not .NET. a simple countdown timer for windows. much worse than hourglass is, but starts instantly.

- can't be paused, can't be stopped, this is intentional. of course it can be closed by pressing alt+f4 or killing the process
- config file can be opened by pressing e
- parses things like 5pm or 7am and counts down to them based on the difference from the current time, but doesn't do "friday 7pm" or similar
- parses 1h as one hour, 50m as 50 minutes, etc. simple numbers will be interpreted as number of seconds
- default font is atkinson hyperlegible next mono light (https://github.com/googlefonts/atkinson-hyperlegible-next-mono/raw/refs/heads/main/fonts/ttf/AtkinsonHyperlegibleMono-Light.ttf)
- g++ -O3 -std=c++17 simplecountdowntimerforwindows.cpp -o simplecountdowntimerforwindows.exe -luser32 -lgdi32 -lwinmm -lshell32 -mwindows -ucrt
- https://winlibs.com/#download-release
- C:\mingw64\bin in path
- %SystemRoot%\System32\accessibilitycpl.dll has a good icon if you want
<img width="1050" height="600" alt="image" src="https://github.com/user-attachments/assets/7b07889c-47ee-4e78-8bbe-728c355f97e2" />
<img width="1050" height="600" alt="image" src="https://github.com/user-attachments/assets/5a808216-0c4f-4a84-9f47-70b9daef6d6f" />
<img width="1050" height="600" alt="image" src="https://github.com/user-attachments/assets/5fa41662-5698-4a45-87c8-1812bc7d9e26" />
