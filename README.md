# Tsuki - 🌙 Live Moon Tracking Application

**Tsuki** is a desktop application that calculates and displays various pieces of lunar information based on your approximate longitude and latitude coordinates. This information is then beautifully rendered using custom-made pixel art.

## ✨ How It Works

1.  **Geolocation**: Your approximate longitude and latitude coordinates are retrieved through a simple HTTPS request to `https://ip-api.com`.
2.  **Lunar Calculations**: Using these retrieved coordinates and your system's current time, `MoonInfo.cpp` calculates the moon's illumination, phase, and your local moonset/moonrise times. These calculations are performed using an ephemeris model of the moon.
3.  **Graphical Display**: All the calculated data is then visualized on your screen using custom-made pixel art, powered by the SFML and ImGui libraries, ensuring cross-platform compatibility.

---

## 🚀 Installation

**Forewarning**: This application was primarily built and tested on a Linux machine, making Linux the easiest environment for setup. I have also performed some testing and refinement for Windows, so it should work there as well. However, I do not have a macOS environment to test on, so I cannot guarantee the success of the installation steps for macOS.

### Prerequisites

You'll need to install **OpenSSL** and **CMake**.

#### 1. OpenSSL

* **Linux**: Install via your package manager.
    ```sh
    sudo pacman -S openssl
    # or
    sudo apt install openssl
    ```
* **macOS**: Ideally, use a package manager like Homebrew. macOS often comes pre-installed with OpenSSL; you can check by entering `openssl version` in your terminal.
    ```sh
    brew install openssl
    ```
* **Windows**: Refer to OpenSSL's official documentation for Windows at [https://github.com/openssl/openssl/blob/master/NOTES-WINDOWS.md](https://github.com/openssl/openssl/blob/master/NOTES-WINDOWS.md). I personally recommend using MSys2 ([https://www.msys2.org/docs/installer/](https://www.msys2.org/docs/installer/)), which offers a straightforward setup and acts as a package manager for most dependencies, including a C++ compiler if you don't already have one.
    ```sh
    # Using MSys2
    pacman -S mingw-w64-x86_64-openssl
    ```

#### 2. CMake

* **Linux**: Install via your package manager.
    ```sh
    sudo pacman -S cmake
    # or
    sudo apt install cmake
    ```
* **macOS**: Again, Homebrew is highly recommended.
    ```sh
    brew install cmake
    ```
* **Windows**: You can either use MSys2 or download CMake directly from [https://cmake.org/download/](https://cmake.org/download/).
    ```sh
    # Using MSys2
    pacman -S mingw-w64-x86_64-cmake
    ```

### Build Steps

Make sure you have Git installed (via your package manager or from [https://git-scm.com/downloads](https://git-scm.com/downloads)).

1.  **Clone the repository**:
    ```sh
    git clone [https://github.com/BHaftner/tsuki.git](https://github.com/BHaftner/tsuki.git)
    ```
2.  **Navigate and Build**:
    ```sh
    cd tsuki
    mkdir build
    cd build
    cmake ..
    make # or 'ninja' depending on your CMake generator
    ```

### Final Notes and Picture

Everything should have hopefully compiled and you should fine a tsuki executable in the projects root directory. It should hopefully run without any issues :)

A really fun project to work on. I plan to keep expanding it where I can! If anyone has any questions, feedback, or problems feel free to contact me at brockhaftner@gmail.com

Cheers!

![image](https://github.com/user-attachments/assets/5b1c1b2d-ebfe-4440-8a9c-5dc628425aef)

