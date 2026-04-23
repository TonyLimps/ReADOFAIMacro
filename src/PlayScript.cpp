#include <Level.h>
#include <PlayScript.h>

// 规范化角度
static double standardizeDegree(double angle) {
	// 冰与火没有0度旋转角，但有360度发卡弯，角度有效范围是(0, 360]
	// angle并不会太大或太小，用while应该比fmod快一些
	while (angle <= 0)
		angle += 360;
	while (angle > 360)
		angle -= 360;
	return angle;
}
// 计算两个普通轨道之间的角度差
static double trackAngleDifference(const double& track1, const double& track2) {
	return standardizeDegree(standardizeDegree(track1 - 180) - track2);
}
// 计算中旋轨道的角度差，第一个参数是中旋轨道角度，第二个正常
static double trackAngleDifferenceMidSpin(const double& midSpin, const double& track2) {
	return standardizeDegree(midSpin - track2);
}

namespace ReADOFAIMacro {

	auto computeRotationAngles(const Level& level) -> std::vector<double> {
		std::vector<double> rotationAngles;
		const size_t floorCount = level.getFloorCount();
		rotationAngles.reserve(floorCount);
		for (int index = 0; index < floorCount - 1; index++) {
			const double alpha1 = level.angleAtFloor(index);
			double alpha2 = level.angleAtFloor(index + 1);
			if (alpha2 == 999) {
				if (index+2 == floorCount) {
					rotationAngles.push_back(999);
					break;
				}
				alpha2 = level.angleAtFloor(index + 2);
				const double theta = trackAngleDifferenceMidSpin(alpha1, alpha2);
				// 保留999，让后面处理事件时rotation_angles能和angleData对齐
				rotationAngles.push_back(999);
				rotationAngles.push_back(theta);
				index++;
			} else {
				const double theta = trackAngleDifference(alpha1, alpha2);
				rotationAngles.push_back(theta);
			}
		}
		return rotationAngles;
	}
	void processTwirl(const Level& level, std::vector<double>& rotationAngles) {
		const size_t size = level.getTwirlRanges().size();
		for (int index = 0; index < size; index++) {
			const auto& range = level.getTwirlRanges().at(index);
			range.forEach([level, &rotationAngles](const size_t floor) {
				double& theta = rotationAngles.at(floor);
				if (theta != 360 && !level.isMidSpin(floor+1)) {
					theta = 360 - theta;
				}
			});
		}
	}
	void processPause(const Level& level, std::vector<double>& rotationAngles) {
		const size_t size = level.getPauses().size();
		for (int index = 0; index < size; index++) {
			const auto& pair = level.getPauses().at(index);
			size_t floor = pair.first;
			if (level.isMidSpin(floor+1)) {
				continue;
			}
			rotationAngles.at(floor) += pair.second * 180;
		}
	}
	void processSetSpeed(const Level& level, std::vector<double>& rotationAngles, double baseBpm) {
		const size_t size = level.getSpeedList().size();
		const double levelBpm = level.getSetting<double>("bpm");
		const double globalMultiplier = baseBpm / levelBpm;

		for (int index = 0; index < size; index++) {
			const auto& pair = level.getSpeedList().at(index);
			const Range& range = pair.first;
			range.forEach([level, &rotationAngles, &pair, levelBpm, globalMultiplier](const size_t floor) {
				if (level.isMidSpin(floor+1)) return;
				const double multiplier = pair.second.first / levelBpm / globalMultiplier;
				rotationAngles.at(floor) /= multiplier;
			});
		}
	}
	auto computeTimeStamps(const Level& level, std::vector<double>& rotationAngles, double bpm) -> std::vector<uint_fast64_t> {
		std::vector<uint_fast64_t> timeStamps;
		const size_t size = rotationAngles.size();
		timeStamps.reserve(size);
		timeStamps.push_back(0);

		double accumulatedMs = 0.0;

		for (int index = 1; index < size; index++) {
			if (level.isMidSpin(index + 1))
				continue;
			const double theta = rotationAngles.at(index);
			const double relMs = theta / 180.0 / bpm * 60.0 * 1000.0;
			accumulatedMs += relMs;
			timeStamps.push_back(static_cast<uint_fast64_t>(std::round(accumulatedMs)));
		}
		return timeStamps;
	}
	PlayScript::PlayScript(const Level& level) {
		double bpm = level.getSetting<double>("bpm");
		std::vector<double> rotationAngles = computeRotationAngles(level);
		processTwirl(level,rotationAngles);
		processPause(level,rotationAngles);
		processSetSpeed(level,rotationAngles,bpm);
		std::vector<uint_fast64_t> floorTimeStamps = computeTimeStamps(level,rotationAngles,bpm);

		const size_t size = floorTimeStamps.size();
		inputs.reserve(size);
		const InputEvent down{.key = &keySequence.p1, .state = true};
		const InputEvent up{.key = &keySequence.p1, .state = false};
		for (int i = 0; i < size-1; i++) {
			const uint_fast64_t downTime = floorTimeStamps[i];
			const uint_fast64_t upTime = (downTime + floorTimeStamps[i+1]) / 2;
			inputs.push_back(down);
			timeStamps.push_back(downTime);
			inputs.push_back(up);
			timeStamps.push_back(upTime);
		}
		inputs.push_back(down);
		inputs.push_back(up);
		const uint_fast64_t downTime = floorTimeStamps[size-1];
		const uint_fast64_t upTime = downTime + timeStamps[size-2] - timeStamps[size-3];
		timeStamps.push_back(downTime);
		timeStamps.push_back(upTime);
	}

}