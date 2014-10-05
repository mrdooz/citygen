#pragma once

void InitGL();
void InitImGui();
void UpdateImGui();

extern bool mousePressed[2];

extern GLFWwindow* g_window;

extern int g_windowWidth;
extern int g_windowHeight;
extern glm::mat4 g_view, g_proj;
extern glm::vec3 g_cameraPos;
