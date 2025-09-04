#pragma once
#include "pch.hpp"

class GraphicsSystem {
public:
    GraphicsSystem();
    ~GraphicsSystem();
    
    bool Initialize(HWND windowHandle, int width, int height);
    void Shutdown();
    void BeginFrame();
    void EndFrame();
    void Resize(int width, int height);
    
    // Core rendering functions
    void SetClearColor(float r, float g, float b, float a);
    void Clear();
    
    // Handle to underlying graphics context/device
    void* GetDeviceContext() { return m_deviceContext; }

private:
    // Implementation-specific data
    void* m_device = nullptr;
    void* m_deviceContext = nullptr;
    int m_width = 0;
    int m_height = 0;
    // Add more members based on chosen API
};