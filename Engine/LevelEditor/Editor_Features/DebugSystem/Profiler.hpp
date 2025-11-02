#include "pch.hpp"
#if defined(PLATFORM_WINDOWS) && defined(_DEBUG)
#include <deque>
#include <string>
#include <unordered_map>
#include "LFeature.hpp"              // base editor feature (defines LFeature)

// Do NOT include Level_Editor headers here.

namespace EngineFramework {
    class LFeature;

    class LFeature_DebugProfiler final : public LFeature {
    public:
        LFeature_DebugProfiler() = default;
        ~LFeature_DebugProfiler() override = default;

        //bool is_visible = true;

        void Initialize() override;
        void Run() override; // Draw window each editor frame (when enabled)
        void Close() override;

    private:
        // Simple rolling buffers
        struct Ring {
            std::deque<float> data;
            int max = 300; // ~5s @60 fps
            void push(float v) { data.push_back(v); if ((int)data.size() > max) data.pop_front(); }
            void clear() { data.clear(); }
        };

        void DrawWindow();

        bool open_window = true; // Panel visibility
        Ring frame_hist; // Frame-ms history
        std::unordered_map<std::string, Ring> section_hist; // % of UpdateAll per label
    };

} // namespace EngineFramework
#endif // _WIN32 && _DEBUG
