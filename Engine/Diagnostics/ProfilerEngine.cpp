#include "ProfilerEngine.hpp"

namespace Diagnostics {

	Profiler& Profiler::I() {
		static Profiler p;
		return p;
	}

	void Profiler::SetEnabled(bool on) { enabled_.store(on, std::memory_order_relaxed); }
	bool Profiler::IsEnabled() const { return enabled_.load(std::memory_order_relaxed); }

	// For the start and end of each game tick
	void Profiler::BeginFrame() {
		if (!IsEnabled()) {
			return;
		}

		std::lock_guard<std::mutex> lk(m_);
		sections_.clear(); // Per frame aggregation - start fresh each frame
		frame_start_ = Clock::now(); // Mark the start time to compute framems() in endframe()
	}

	void Profiler::EndFrame() {
		if (!IsEnabled()) {
			return;
		}
		std::lock_guard<std::mutex> lk(m_);
		const auto now = Clock::now();
		frame_ms_ = std::chrono::duration<double, std::milli>(now - frame_start_).count();

		// TESTING FPS FOR NOW
		const double inst = frame_ms_ > 0.0 ? 1000.0 / frame_ms_ : 0.0;
		fps_ = (fps_ == 0.0) ? inst : (fps_ * 0.9 + inst * 0.1);
	}

	// Recording the data of the section for the profiler
	void Profiler::AddSample(const char* name, double ms) {
		if (!IsEnabled()) {
			return;
		}
		std::lock_guard<std::mutex> lk(m_);

		// To create the entry on first use
		auto& section = sections_[name];
		section.ms += ms;
		section.calls += 1;
	}

	void Profiler::Reset() {
		std::lock_guard<std::mutex> lk(m_);
		sections_.clear();
		frame_ms_ = 0.0;
		fps_ = 0.0;
	}

	Scoped::Scoped(const char* name) : name_(name), t0_(Clock::now()) {}

	Scoped::~Scoped() {
		const auto t1 = Clock::now();
		const double ms = std::chrono::duration<double, std::milli>(t1 - t0_).count();
		Profiler::I().AddSample(name_, ms);
	}
}