
# 3D Classroom Simulation Engine

![Language](https://img.shields.io/badge/language-C%2B%2B17-blue) ![OpenGL](https://img.shields.io/badge/OpenGL-3.3%20Core-green) ![Platform](https://img.shields.io/badge/platform-Linux%20%7C%20Windows-lightgrey) ![License](https://img.shields.io/badge/license-MIT-orange)

A high-performance, real-time rendering engine simulating a classroom environment. Built from scratch using **C++** and **modern OpenGL 3.3 (Core Profile)**.

This project was developed to demonstrate proficiency in low-level graphics programming, rendering pipeline architecture, and the application of linear algebra in 3D simulations.

## 📸 Demo
*(will be updated soon)*

---

## 🚀 Key Technical Features

### 1. Multipass Shadow Mapping
* **Technique:** Implemented a **Render-To-Texture (RTT)** pipeline. The first pass renders scene depth from the light's perspective; the second pass renders the scene using that depth map for lighting calculations.
* **Optimization:** Utilizes a `GL_TEXTURE_2D_ARRAY` to store shadow maps for **9 distinct light sources** simultaneously. This allows all shadows to be calculated in a single shader pass, significantly reducing draw call overhead.
* **Filtering:** Hardware **Percentage-Closer Filtering (PCF)** enabled to smooth shadow edges and reduce aliasing.

### 2. Advanced Material Rendering
* **Normal Mapping:** Calculates **Tangent Space (TBN Matrices)** to simulate high-frequency surface details (bumps and dents) on low-poly geometry like brick walls.
* **Specular Highlights:** Implements the Blinn-Phong lighting model with specific specular maps to define material shininess (e.g., dull wood vs. shiny metal).
* **Transparency:** Features Alpha Blending with manual depth sorting to correctly render transparent objects (glass windows) on top of opaque geometry.

### 3. Architecture & Optimization
* **Object-Oriented Design:** The engine is encapsulated in a `ClassroomSimulator` class, which manages the lifecycle of the OpenGL context, assets, and the main game loop.
* **Hardware Instancing:** High-volume objects (e.g., 25 benches, 6 fans) are rendered using `glDrawElementsInstanced`. This technique draws hundreds of copies of a mesh with a single API call.
* **Data-Driven Design:** The render loop utilizes categorized buckets (`opaque`, `transparent`, `normal_mapped`) to minimize state changes and streamline the pipeline.

---

## 📂 Project Structure

The project is organized into a modular structure to separate core logic, shaders, and assets.

```text
OpenGL-Classroom-Simulation/
├── CMakeLists.txt          # Build configuration (CMake)
├── README.md               # Project documentation
├── src/
│   └── main.cpp            # Core engine logic & ClassroomSimulator class
├── shaders/
│   ├── ShadowMapping.* # Main lighting shaders (Vertex/Fragment)
│   └── DepthRTT.* # Shadow generation shaders
├── common/                 # Helper library (Shader loader, OBJ loader)
│   ├── shader.cpp
│   ├── controls.cpp
│   └── ...
└── assets/                 # 3D Resources
    ├── models/             # Wavefront .obj files (benches, walls, etc.)
    └── textures/           # .dds and .bmp texture maps

```

---

## 💻 Setup & Build Instructions

### Prerequisites

Ensure you have the following installed on your system:

* **C++ Compiler:** GCC (Linux) or MSVC (Windows)
* **CMake:** Version 3.10 or higher
* **Libraries:**
* `GLFW3` (Window Manager)
* `GLEW` (OpenGL Extension Wrangler)
* `GLM` (OpenGL Mathematics)



### Option 1: Linux (Terminal)

1. **Clone the repository:**
```bash
git clone [https://github.com/YOUR_USERNAME/OpenGL-Classroom-Simulation.git](https://github.com/YOUR_USERNAME/OpenGL-Classroom-Simulation.git)
cd OpenGL-Classroom-Simulation

```


2. **Create a build directory:**
```bash
mkdir build
cd build

```


3. **Compile the project:**
```bash
cmake ..
make

```


4. **Run the simulation:**
```bash
./ClassroomRenderer

```


*Note: If the app closes immediately, ensure the `assets/` folder is copied to the build directory.*

### Option 2: Windows (Visual Studio)

1. Open **Visual Studio**.
2. Select **"Open a local folder"** and choose the project root folder.
3. Visual Studio should detect `CMakeLists.txt` and configure automatically.
4. Select `ClassroomRenderer.exe` from the startup dropdown menu.
5. Press **F5** to build and run.

---

## 🎮 Controls

The simulation uses standard First-Person controls.

| Input | Action |
| --- | --- |
| **W, A, S, D** | Move Camera (Forward, Left, Back, Right) |
| **Mouse** | Look Around |
| **G** | Toggle Shading Mode (Switch between Gouraud and Phong) |
| **ESC** | Exit Application |

---

## 📚 Acknowledgments & Credits

* **Framework:** Base loader utilities adapted from [opengl-tutorial.org](http://www.opengl-tutorial.org/).
* **Assets:** Classroom models sourced from educational 3D repositories.
* **Author:** Moksham.

---

