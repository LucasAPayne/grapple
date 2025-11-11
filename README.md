# Grapple

Grapple is a tool for quickly opening VS Code projects using only the keyboard. Press `Ctrl`+`Shift`+`Space` to bring Grapple to the foreground, type the name of your project, and press `Enter` to open the project in VS Code.

## Features

- Quickly open VS Code projects from anywhere using only the keyboard
- Lightweight and runs silently in the background
- Simple INI configuration for project shortcuts

## Demo

![Demo Video](doc/images/grapple_demo.mp4)

## Installation

### Requirements

Grapple requires at least Windows 10 and a device compatible with Direct3D 11. Currently, Grapple must be built from source with the MSVC compiler (`cl`). This requires locating and running `vcvarsall.bat` or a terminal like x64 Native Tools Command Prompt for VS 2022.

### Building Grapple

Clone the repository or download a ZIP archive of it and extract it. Navigate to the `grapple` folder and run the `build.bat` script, which will produce `build/grapple.exe`.

```bat
git clone https://github.com/LucasAPayne/grapple
cd grapple
build
```

> [!NOTE]
> Run `build.bat` from an environment where the MSVC compiler (`cl`) is available, such as x64 Native Tools Command Prompt for VS 2022.

### Setting Grapple as a Startup Process

It is recommended to set `grapple.exe` as a startup process. To do so, right-click this file and select `Create Shortcut`, and place the shortcut in the folder `%userprofile%\AppData\Roaming\Microsoft\Windows\Start Menu\Programs\Startup`. The next time your computer is restarted, Grapple should be visible in the system tray, and you should be able to use the keyboard shortcut to bring it to the foreground.

## How to Use Grapple

Once Grapple is installed, you can register the paths to your VS Code projects and the names you want to associate with them in the `config/grapple.ini` file. This file already exists in the project with some examples. The format of that file looks something like this:

```ini
[Projects]
projectA = relative/path/to/project
projectB = C:/absolute/path/to/project
```

For each project, the string on the left-hand side of the equal sign is what you will type into the search bar to open the project, and the path to the project goes on the right-hand side.

> [!NOTE]
> The format of this configuration file will likely change throughout development.

## Planned Features

- Configurable keyboard shortcut
- Optional project properties (activate `vcvarsall.bat` for `cl`, connect to a remote project, etc.)
- Improved text box input (keyboard shortcuts, mouse/keyboard text selection)
- GUI for settings and project configuration

## More About Grapple

See more information about the project [on my website](https://lucasapayne.com/projects/grapple).
