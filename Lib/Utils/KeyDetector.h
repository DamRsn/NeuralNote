//
// Key detection from melodic note durations (Krumhansl-Schmuckler).
//

#ifndef NN_KEYDETECTOR_H
#define NN_KEYDETECTOR_H

#include <array>
#include <cmath>
#include <cstddef>
#include <optional>
#include <string>

namespace KeyDetector
{
enum class Mode {
	Major,
	Minor
};

struct DetectedKey {
	int rootPitchClass = 0; // 0 = C .. 11 = B
	Mode mode = Mode::Major;
	double correlation = 0.0;

	bool operator==(const DetectedKey&) const = default;

	bool hasSameKey(const DetectedKey& inOther) const
	{
		return rootPitchClass == inOther.rootPitchClass && mode == inOther.mode;
	}

	const char* rootName() const
	{
		static constexpr std::array<const char*, 12> MAJOR_NAMES {
			"C", "Db", "D", "Eb", "E", "F", "F#", "G", "Ab", "A", "Bb", "B"};
		static constexpr std::array<const char*, 12> MINOR_NAMES {
			"C", "C#", "D", "Eb", "E", "F", "F#", "G", "G#", "A", "Bb", "B"};

		const auto index = static_cast<std::size_t>(((rootPitchClass % 12) + 12) % 12);
		return mode == Mode::Major ? MAJOR_NAMES[index] : MINOR_NAMES[index];
	}

	std::string format() const
	{
		return std::string(rootName()) + (mode == Mode::Major ? " maj" : " min");
	}
};

inline constexpr int MIN_DISTINCT_PITCH_CLASSES = 3;
inline constexpr double MIN_CORRELATION = 0.0;

// Krumhansl-Kessler (1982) major and minor probe-tone key profiles, indexed C..B.
inline constexpr std::array<double, 12> MAJOR_PROFILE {
	6.35, 2.23, 3.48, 2.33, 4.38, 4.09, 2.52, 5.19, 2.39, 3.66, 2.29, 2.88};
inline constexpr std::array<double, 12> MINOR_PROFILE {
	6.33, 2.68, 3.52, 5.38, 2.60, 3.53, 2.54, 4.75, 3.98, 2.69, 3.34, 3.17};

inline double pearsonCorrelation(const std::array<double, 12>& inWeights,
	const std::array<double, 12>& inProfile,
	int inRootPitchClass)
{
	double sum_x = 0.0;
	double sum_y = 0.0;

	for (int i = 0; i < 12; ++i) {
		sum_x += inWeights[static_cast<std::size_t>(i)];
		sum_y += inProfile[static_cast<std::size_t>(i)];
	}

	const double mean_x = sum_x / 12.0;
	const double mean_y = sum_y / 12.0;

	double sum_xy = 0.0;
	double sum_xx = 0.0;
	double sum_yy = 0.0;

	for (int pitch_class = 0; pitch_class < 12; ++pitch_class) {
		const int degree = (pitch_class - inRootPitchClass + 12) % 12;
		const double dx = inWeights[static_cast<std::size_t>(pitch_class)] - mean_x;
		const double dy = inProfile[static_cast<std::size_t>(degree)] - mean_y;

		sum_xy += dx * dy;
		sum_xx += dx * dx;
		sum_yy += dy * dy;
	}

	if (sum_xx <= 0.0 || sum_yy <= 0.0) {
		return 0.0;
	}

	return sum_xy / std::sqrt(sum_xx * sum_yy);
}

inline std::optional<DetectedKey> detectFromPitchClassWeights(const std::array<double, 12>& inWeights)
{
	int distinct_pitch_classes = 0;

	for (const double weight: inWeights) {
		if (!std::isfinite(weight) || weight < 0.0) {
			return std::nullopt;
		}
		if (weight > 0.0) {
			++distinct_pitch_classes;
		}
	}

	if (distinct_pitch_classes < MIN_DISTINCT_PITCH_CLASSES) {
		return std::nullopt;
	}

	std::optional<DetectedKey> best_key;
	double best_correlation = MIN_CORRELATION;

	for (int root = 0; root < 12; ++root) {
		const double major_r = pearsonCorrelation(inWeights, MAJOR_PROFILE, root);
		if (major_r > best_correlation) {
			best_correlation = major_r;
			best_key = DetectedKey {root, Mode::Major, major_r};
		}

		const double minor_r = pearsonCorrelation(inWeights, MINOR_PROFILE, root);
		if (minor_r > best_correlation) {
			best_correlation = minor_r;
			best_key = DetectedKey {root, Mode::Minor, minor_r};
		}
	}

	return best_key;
}

template <typename NoteContainer, typename AudiblePred>
inline std::optional<DetectedKey> detect(const NoteContainer& inNotes, AudiblePred&& inIsProgramAudible)
{
	std::array<double, 12> weights {};

	for (const auto& note: inNotes) {
		if (note.isDrum() || !inIsProgramAudible(note.program)) {
			continue;
		}
		if (note.pitch < 0 || note.pitch > 127) {
			continue;
		}

		const double duration = note.endTime - note.startTime;
		if (!std::isfinite(duration) || duration <= 0.0) {
			continue;
		}

		const int pitch_class = note.pitch % 12;
		weights[static_cast<std::size_t>(pitch_class)] += duration;
	}

	return detectFromPitchClassWeights(weights);
}

template <typename NoteContainer>
inline std::optional<DetectedKey> detect(const NoteContainer& inNotes)
{
	return detect(inNotes, [](int) { return true; });
}
} // namespace KeyDetector

#endif // NN_KEYDETECTOR_H
