#include "pch.hpp"
#if defined(PLATFORM_WINDOWS) && defined(_DEBUG)

#include "Profiler.hpp"
#include "Diagnostics/ProfilerEngine.hpp" // Engine side profiler
#include <algorithm>
#include <imgui.h>

// Generic getter for ImGui::PlotLines using any []-indexable container
template <typename Container>
static float ImGuiDequeGetter(void* data, int idx) {
    return (float)(*static_cast<const Container*>(data))[idx];
}

// RAII guard for ImGui window
struct ImGuiWindowGuard {
    bool open = false;
    ImGuiWindowGuard(const char* title, bool* p_open = nullptr) { open = ImGui::Begin(title, p_open); }
    ~ImGuiWindowGuard() { ImGui::End(); }
    explicit operator bool() const { return open; }
};

static std::unordered_map<std::string, double> s_ema_ms; // To store a smoothed execution time for each section using exponential moving average (EMA) (dont use raw execution time directly since it bounces frame to frame)
static std::unordered_map<std::string, int> s_index;  // To remember to last displayed order index of each section so rows wont be swapped unless there is a clear reason
static std::vector<std::string> s_order;  // A persistent list of section names in the order to be displayed
static double s_ema_updateAll = 0.0; // A smoothed denominator variable
static double s_ema_frame_ms = 0.0; // A smoothed frame time for display stability
static double s_last_sort_time = 0.0; // Timestamp of the last time the rows are reordered, only do so if enough time has passed to avoid rearranging every frame
static bool   s_freeze_order = false; // If true, never re order

constexpr double EMA_ALPHA = 0.08; // Exponential Moving Average blend factor (from 0 to 1), the lower the factor, the steadier the bar gets - reduced for more stability
constexpr double RESORT_PERIOD = 1.00; // The minimum time between each reorderings
constexpr double RERANK_HYST = 0.30; // Hysteresis for swapping the neighbours, so neighbour B only overtakes A after a certain condition is met

static bool s_auto_sort = false; // To prevent list from being sorted
static bool s_sort_once = false; // To trigger a one shot re sort from UI

constexpr int MS_PRECISION = 6; // Precision value for the ms to be shown in profiler

// Ensure label exists in s_order at a deterministic spot (alphabetical if new)
static void EnsureLabelInOrder(const std::string& name) {
    if (s_index.count(name)) return;
    // Alphabetical insertion point
    auto it = std::lower_bound(s_order.begin(), s_order.end(), name);
    int pos = (int)std::distance(s_order.begin(), it);
    s_order.insert(it, name);
    // Rebuild indices from pos onward
    for (int i = pos; i < (int)s_order.size(); ++i) s_index[s_order[i]] = i;
}

namespace EngineFramework {
    void LFeature_DebugProfiler::Initialize() {}

    void LFeature_DebugProfiler::Run() {
        if (!is_enabled) {
            return;
        }
        DrawWindow();
    }

    void LFeature_DebugProfiler::Close() {
        open_window = false;
    }

    void LFeature_DebugProfiler::DrawWindow() {
        auto& P = Diagnostics::Profiler::I();

        constexpr const char* kProfilerWin = "Profiler##LevelEditor";
        ImGuiWindowGuard win(kProfilerWin, &is_enabled);
        if (!win) {
            return;
        }

        if (!P.IsEnabled()) {
            ImGui::TextDisabled("Engine profiler is disabled.");
            if (ImGui::Button("Enable profiler")) P.SetEnabled(true);
            return;
        }

        const double frame_ms = P.FrameMS();
        const double fps = P.FPS();

        // Smooth frame time for display stability
        s_ema_frame_ms = (s_ema_frame_ms == 0.0) ? frame_ms : s_ema_frame_ms * (1.0 - EMA_ALPHA) + frame_ms * EMA_ALPHA;
        s_ema_frame_ms = std::max(s_ema_frame_ms, 0.001);

        frame_hist.push(static_cast<float>(frame_ms));

        // Copy and sort the sections by total ms in descending order
        //std::vector<std::pair<std::string, Diagnostics::Section>> rows(P.Sections().begin(), P.Sections().end());
        //std::sort(rows.begin(), rows.end(), [](auto& a, auto& b) {return a.second.ms > b.second.ms; });

        const auto& secs = P.Sections();
        // Make sure every visible label has a deterministic position
        for (const auto& kv : secs) {
            EnsureLabelInOrder(kv.first);
        }

        // Choose denominator (prefer Frame::UpdateAll, but be consistent)
        double denominator = frame_ms;
        if (auto iterator = secs.find("Frame::UpdateAll"); iterator != secs.end() && iterator->second.ms > 0.001) {
            denominator = iterator->second.ms;
        }

        // Ensure minimum denominator to prevent extreme values
        denominator = std::max(denominator, 0.001);

        // Smooth denominator
        s_ema_updateAll = (s_ema_updateAll == 0.0) ? denominator : s_ema_updateAll * (1.0 - EMA_ALPHA) + denominator * EMA_ALPHA;
        s_ema_updateAll = std::max(s_ema_updateAll, 0.001); // Ensure smoothed denominator is also bounded

        // Update history + per-label EMA
        for (const auto& kv : secs) {
            const std::string& name = kv.first;
            const auto& sec = kv.second;

            // sparkline history uses instantaneous % of smoothed denom for consistency
            float percent_update = (float)((sec.ms / s_ema_updateAll) * 100.0);
            auto& ring = section_hist[name];
            ring.max = frame_hist.max;
            ring.push(percent_update);

            // EMA for calmer bars & ordering - initialize with smoothed value to avoid jumps
            if (!s_ema_ms.count(name)) {
                // Initialize new sections with current smoothed denominator ratio to avoid initial spikes
                s_ema_ms[name] = sec.ms;
            }
            double sm = s_ema_ms[name];
            sm = sm * (1.0 - EMA_ALPHA) + sec.ms * EMA_ALPHA;
            s_ema_ms[name] = sm;
        }

        // Build the rows strictly using the sticky order so hash-order can't jitter it
        std::vector<std::string> rows; rows.reserve(s_order.size());
        for (const auto& name : s_order) {
            if (secs.find(name) != secs.end()) rows.push_back(name);
        }

        // Allow overtakes with hysteresis at most a few times/second
        const double now = ImGui::GetTime();
        // If the reordering isn't freeze and enough time has passed since the the last reorddering, then reorder can be considered
        if (!s_freeze_order && (s_sort_once || (s_auto_sort && (now - s_last_sort_time > RESORT_PERIOD)))) {
            bool changed = false;

            // Returns the smoothed ms for the particular section
            auto score = [](const std::string& n) {
                auto it = s_ema_ms.find(n);
                return (it != s_ema_ms.end()) ? it->second : 0.0;
                };

            // A few bubble passes are enough and preserve locality using hysteresis
            for (int pass = 0; pass < 3; ++pass) {
                for (size_t i = 1; i < rows.size(); ++i) {
                    double prev = score(rows[i - 1]);
                    double curr = score(rows[i]);
                    if (curr > prev * (1.0 + RERANK_HYST)) {
                        std::swap(rows[i - 1], rows[i]);
                        changed = true;
                    }
                }
            }

            if (changed) {
                std::vector<std::string> new_order = rows; // Present the section labels first
                new_order.reserve(s_order.size());
                for (const auto& name : s_order) { // The append thje absent section albels
                    if (secs.find(name) == secs.end()) {
                        new_order.push_back(name);
                    }
                }
                s_order.swap(new_order);

                // Rebuild indices
                s_index.clear();
                for (int i = 0; i < (int)s_order.size(); ++i) {
                    s_index[s_order[i]] = i;
                }

                s_last_sort_time = now;
                s_sort_once = false;
            }
        }

        // Header / legend
        ImGui::Text("Frame: %.3f ms  (FPS %.1f)   UpdateAll (smoothed): %.3f ms", s_ema_frame_ms, fps, s_ema_updateAll);
        ImGui::TextDisabled("Bars = smoothed %% of UpdateAll; label shows current ms.");
        ImGui::Checkbox("Freeze order", &s_freeze_order);

        ImGui::SameLine();
        ImGui::Checkbox("Auto-Sort (slow)", &s_auto_sort);
        ImGui::SameLine();
        if (ImGui::Button("Sort once")) {
            s_sort_once = true;
        }

        // Frame-time graph
        if (!frame_hist.data.empty()) {
            ImGui::PlotLines("Frame (ms)",
                ImGuiDequeGetter<std::deque<float>>, (void*)&frame_hist.data,
                (int)frame_hist.data.size(), 0, nullptr, 0.0f, 33.0f, ImVec2(-1, 80));
        }

        ImGui::SeparatorText("Top sections (% of UpdateAll)");
        const int topN = 8;
        int shown = 0;

        for (const auto& name : rows) {
            const auto& sec = secs.at(name);

            if (shown++ >= topN) break;

            const double smoothed = s_ema_ms[name];
            const double pctUpd = (smoothed / s_ema_updateAll) * 100.0;
            const double pctFrame = (smoothed / s_ema_frame_ms) * 100.0;
            //const double per_call = sec.calls ? sec.ms / sec.calls : 0.0;

            //// hide trivial noise unless hovered
            //if (pctUpd < 0.5 && sec.ms < 0.02 && !ImGui::IsWindowHovered())
            //    continue;

            float frac = (float)(smoothed / s_ema_updateAll);
            if (frac > 1.f) frac = 1.f;

            // Use smoothed values for stable display - remove jumping instantaneous values
            char label[256];
            snprintf(label, sizeof(label), "%s  %d%%  %.*f ms", name.c_str(), (int)pctUpd, MS_PRECISION, smoothed);
            ImGui::ProgressBar(frac, ImVec2(-1, 0), label);

            // sparkline: instantaneous % history
            auto& ring = section_hist[name].data;
            if (!ring.empty()) {
                ImGui::PlotLines(("##hist_" + name).c_str(),
                    ImGuiDequeGetter<std::deque<float>>,
                    (void*)&ring, (int)ring.size(),
                    0, nullptr, 0.0f, 100.0f, ImVec2(-1, 32));
            }

            ImGui::TextDisabled("calls %d, %.*f ms/call  (%.1f%% of Frame)", sec.calls, MS_PRECISION, (sec.calls ? smoothed / sec.calls : 0.0), pctFrame);
            ImGui::Separator();
        }

        // Full table
        if (ImGui::CollapsingHeader("All sections", ImGuiTreeNodeFlags_DefaultOpen)) {
            if (ImGui::BeginTable("prof_tbl", 5, ImGuiTableFlags_RowBg | ImGuiTableFlags_Borders | ImGuiTableFlags_SizingStretchProp)) {
                ImGui::TableSetupColumn("Section");
                ImGui::TableSetupColumn("ms (inst)");
                ImGui::TableSetupColumn("calls");
                ImGui::TableSetupColumn("ms/call");
                ImGui::TableSetupColumn("%Upd / %Frame (smoothed)");
                ImGui::TableHeadersRow();

                for (const auto& name : rows) {
                    const auto& sec = secs.at(name);
                    const double smoothed = s_ema_ms[name];

                    ImGui::TableNextRow();
                    ImGui::TableSetColumnIndex(0); ImGui::TextUnformatted(name.c_str());
                    ImGui::TableSetColumnIndex(1); ImGui::Text("%.*f", MS_PRECISION, smoothed);
                    ImGui::TableSetColumnIndex(2); ImGui::Text("%d", sec.calls);
                    ImGui::TableSetColumnIndex(3); ImGui::Text("%.*f", MS_PRECISION, (sec.calls ? smoothed / sec.calls : 0.0));
                    ImGui::TableSetColumnIndex(4); ImGui::Text("%.1f%% / %.1f%%",
                        (smoothed / s_ema_updateAll) * 100.0,
                        (smoothed / s_ema_frame_ms) * 100.0);
                }
                ImGui::EndTable();
            }
        }

        if (ImGui::Button("Reset")) {
            P.Reset();
            frame_hist.clear();
            section_hist.clear();
            s_ema_ms.clear();
            // keep s_order/s_index so names stay in the same human order after reset,
            // but if you want a hard reset, uncomment the two lines below:
            // s_order.clear();
            // s_index.clear();
            s_ema_updateAll = 0.0;
            s_ema_frame_ms = 0.0;
            s_last_sort_time = 0.0;
        }
        ImGui::SameLine();
        bool capture = Diagnostics::Profiler::I().IsEnabled();
        if (ImGui::Checkbox("Capture timings", &capture)) {
            Diagnostics::Profiler::I().SetEnabled(capture);
        }
    }
} // namespace EngineFramework
#endif


