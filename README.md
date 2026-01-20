# 3D Classroom Simulation Engine

![Language](https://img.shields.io/badge/language-C%2B%2B17-blue) ![OpenGL](https://img.shields.io/badge/OpenGL-3.3%20Core-green) ![License](https://img.shields.io/badge/license-MIT-lightgrey)

A real-time, modular rendering engine simulating a classroom environment. Built from scratch using **C++** and **modern OpenGL 3.3 (Core Profile)**.

This project demonstrates proficiency in low-level graphics programming, rendering pipeline architecture, and linear algebra application in 3D space.

## 📸 Screenshots
*(Placeholder: Drag and drop a screenshot of your classroom scene here)*

## 🚀 Key Technical Features

### 1. Multipass Shadow Mapping
* **Technique:** Implemented a **Render-To-Texture (RTT)** pipeline. The first pass renders depth from the light's perspective; the second pass renders the scene using that depth map.
* **Optimization:** Utilizes a `GL_TEXTURE_2D_ARRAY` to handle shadow maps for **9 distinct light sources** in a single shader pass, avoiding expensive texture rebinding operations.
* **Filtering:** Hardware Percentage-Closer Filtering (PCF) enabled for smoother shadow edges.

### 2. Advanced Material Rendering
* **Normal Mapping:** Computes **Tangent Space (TBN Matrices)** to simulate high-frequency surface detail on low-poly geometry (e.g., brick walls, ceiling tiles).
* **Specular Highlights:** Blinn-Phong lighting model with specular maps for realistic material shininess.
* **Transparency:** Implemented Alpha Blending with manual depth sorting to correctly render glass surfaces over opaque geometry.

### 3. Architecture & Optimization
* **Object-Oriented Design:** The engine is encapsulated in a `ClassroomSimulator` class, managing the lifecycle of the OpenGL context, assets, and game loop.
* **Hardware Instancing:** High-volume objects (e.g., 25 benches, 6 fans) are rendered using `glDrawElementsInstanced`, reducing CPU-GPU draw call overhead significantly.
* **Data-Driven Design:** Rendering logic uses categorized asset buckets (`opaque`, `transparent`, `normal_mapped`) to streamline the render loop.

## 🛠️ Software Architecture

The codebase utilizes a modular architecture to separate initialization, logic, and cleanup:

```cpp
int main() {
    ClassroomSimulator app; // RAII-compliant resource management
    app.Run();              // Encapsulated Game Loop
    return 0;
}
