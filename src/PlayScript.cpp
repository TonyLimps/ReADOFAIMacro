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

	std::vector<uint_fast64_t> getFloorTimeStamps(const Level& level) {
		std::vector<double> rotationAngles;
		const size_t floorCount = level.getFloorCount();
		double bpm = level.getSetting<double>("bpm");
		rotationAngles.reserve(floorCount);
		{
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
					// 保留999，让后面处理事件时rotation_angles能和angleData_对齐
					rotationAngles.push_back(999);
					rotationAngles.push_back(theta);
					index++;
				} else {
					const double theta = trackAngleDifference(alpha1, alpha2);
					rotationAngles.push_back(theta);
				}
			}
		}

		// 处理事件
		{
			size_t size = level.getTwirlRanges().size();
			for (int index = 0; index < size; index++) {
				const auto& range = level.getTwirlRanges().at(index);
				range.forEach([level, &rotationAngles](const size_t floor) {
					double& theta = rotationAngles.at(floor);
					if (theta != 360 && !level.isMidSpin(floor+1)) {
						theta = 360 - theta;
					}
				});
			}

			size = level.getPauses().size();
			for (int index = 0; index < size; index++) {
				const auto& pair = level.getPauses().at(index);
				size_t floor = pair.first;
				if (level.isMidSpin(floor+1)) {
					continue;
				}
				rotationAngles.at(floor) += pair.second * 180;
			}

			size = level.getSpeedList().size();
			for (int index = 0; index < size; index++) {
				const auto& pair = level.getSpeedList().at(index);
				const Range& range = pair.first;
				range.forEach([level, &rotationAngles, &pair, bpm](const size_t floor) {
					if (level.isMidSpin(floor+1)) return;
					const double multiplier = pair.second.first / bpm;
					rotationAngles.at(floor) /= multiplier;
				});
			}
		}
		// 将旋转角转换为相对第一次点击时间的毫秒时间戳
		const size_t size = rotationAngles.size();
		std::vector<uint_fast64_t> timeStamps;
		timeStamps.reserve(size);
		timeStamps.push_back(0);

		double accumulatedMs = 0.0;

		for (int index = 1; index < size; index++) {
			if (level.isMidSpin(index+1)) continue;
			const double theta = rotationAngles.at(index);
			const double relMs = theta / 180.0 / bpm * 60.0 * 1000.0;
			accumulatedMs += relMs;
			timeStamps.push_back(static_cast<uint_fast64_t>(std::round(accumulatedMs)));
		}

		return timeStamps;
	}

	PlayScript::PlayScript(const Level& level, const std::vector<VK>& virtualKeys) {
		const std::vector<uint_fast64_t>& floorTimeStamps = getFloorTimeStamps(level);
		const size_t floorTimeStampsSize = floorTimeStamps.size();
		const size_t keyCount = virtualKeys.size();
		for (int i = 0; i < keyCount; i++) {
			inputs.emplace_back();
		}

		for (size_t i = 1; i < floorTimeStampsSize - 1; i++) {
			const size_t keyIndex = i % keyCount;
			const VK vk = virtualKeys.at(keyIndex);
			const uint_fast64_t downTime = floorTimeStamps.at(i);

			InputEvent down = {};
			down.state = true;
			down.key = vk;
			down.time = downTime;
			inputs.at(keyIndex).push_back(down);

			const uint_fast64_t upTime = (floorTimeStamps.at(i) + floorTimeStamps.at(i + 1)) / 2;
			InputEvent up = {};
			up.state = false;
			up.key = vk;
			up.time = upTime;
			inputs.at(keyIndex).push_back(up);
		}

		if (floorTimeStampsSize > 1) {
			const size_t lastIndex = floorTimeStampsSize - 1;
			const size_t keyIndex = lastIndex % keyCount;
			const VK vk = virtualKeys.at(keyIndex);

			const uint_fast64_t downTime = floorTimeStamps.at(lastIndex);
			InputEvent down = {};
			down.state = true;
			down.key = vk;
			down.time = downTime;
			inputs.at(keyIndex).push_back(down);
		}
	}


}