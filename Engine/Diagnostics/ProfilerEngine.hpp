#pragma once

#include <chrono>
#include <string>
#include <unordered_map>
#include <mutex>

namespace Diagnostics {

	// Summarized stats for a labeled section within the current frame
	struct Section {
		double ms = 0.0f; // Total inclusive time in this frame
		int calls = 0; // Number of times this particular label was recorded in this frame
	};

	class Profiler {
	public:
		static Profiler& I();

		void SetEnabled(bool on);
		bool IsEnabled() const;

		//Frame boundary
		void BeginFrame(); // To start a new frame and clear per-frame stats
		void EndFrame(); // To finalize the frame_ms

		// To record a timing measurement under a specific label, used by Scoped
		void AddSample(const char* name, double ms); // Accumulate the timing for a named section

		void Reset(); // Clear all data and counters

		// Per frame sections map (label -> section) which is valid after endframe is called
		const std::unordered_map<std::string, Section>& Sections() const { return sections_; }

		// Getter for the duration of the last completed frame
		double FrameMS() const { return frame_ms_; }

		// TESTING
		double FPS() const { return fps_; }

	private:
		using Clock = std::chrono::high_resolution_clock;

		// Per frame accumulation
		std::unordered_map<std::string, Section> sections_;

		// Time stamp set by beginframe function
		Clock::time_point frame_start_{};

		// Completed frame duration in milliseconds
		double frame_ms_ = 0.0;
		double fps_ = 0.0f;

		// Run time toggle
		std::atomic<bool> enabled_{ true };

		// Use mutex to guard sections and frame stats
		mutable std::mutex m_;
	};

	// RAII scope timer
	class Scoped {
	public:
		// To time a labeled code region
		explicit Scoped(const char* name);

		// Stop timing and record the sample into the profiler
		~Scoped();

	private:
		using Clock = std::chrono::high_resolution_clock;
		const char* name_; // Label
		Clock::time_point t0_; // Start timestamp
	};
}