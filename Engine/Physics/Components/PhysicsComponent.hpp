#pragma once
struct PhysicsComponent {
    float linear_damping{ 0.0f };
    float angular_damping{ 0.0f };
    bool  ccd{ false };              // continuous collision
    float gravity_factor{ 1.0f };    // 0 disables gravity
    // (extend as needed)
};

