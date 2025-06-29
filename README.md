S# Tsuki - 🌙 Live Moon Tracking Application

**Tsuki** is a desktop application that calculates and displays various pieces of lunar information based on your approximate longitude and latitude coordinates. This information is then rendered using custom-made pixel art.

## ✨ How It Works

1.  **Geolocation**: Uses a .json file generated from GeoNames.org containing cities with a population > 1000 for latitude and longitude information. User simply choose their location.
2.  **Lunar Calculations**: Using these retrieved coordinates and your system's current time, `MoonInfo.cpp` calculates the moon's illumination, phase, and your local moonset/moonrise times. These calculations are performed using an ephemeris model of the moon. NOTE: When selecting other locations, the Moonrise/Moonset times are in your local time.
3.  **Graphical Display**: All the calculated data is then visualized on your screen using custom-made pixel art, powered by SFML to ensure cross-platform compatibility.

---

## 🚀 Installation

**Forewarning**: This application was primarily built and tested on a Linux machine, making Linux the easiest environment for setup. I have also performed some testing and refinement for Windows, so it should work there as well. However, I do not have a macOS environment to test on, so I cannot guarantee the success of the installation steps for macOS.

Please be aware there are statically linked releases for Windows and Linux. This is easiest setup if you have no interest in the codebase. The statically linked versions can be found under the github release tab.

### Prerequisites

You'll need to install **CMake**.

#### CMake

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

Everything should have hopefully compiled and you should now have an executable in the projects root directory. It hopefully runs without any issues :)

A really fun project to work on, I plan to keep expanding it where I can! If anyone has any questions, feedback, or problems feel free to contact me at brockhaftner@gmail.com

Cheers!

![image](https://github.com/user-attachments/assets/8ecd2cb3-c743-484a-b010-17da9fb6313d)
