#include <Level.h>
#include <PlayScript.h>

#include <limits>

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
	auto computeTimeStamps(std::vector<double>& rotationAngles, double bpm) -> std::vector<uint_fast64_t> {
		std::vector<uint_fast64_t> timeStamps;
		rotationAngles.erase(rotationAngles.begin());
		const size_t size = rotationAngles.size();
		timeStamps.reserve(size+1);
		timeStamps.push_back(0);

		double accumulatedMs = 0.0;

		for (int index = 0; index < size; index++) {
			const double theta = rotationAngles[index];
			const double relMs = theta / 180.0 / bpm * 60.0 * 1000.0;
			accumulatedMs += relMs;
			timeStamps.push_back(static_cast<uint_fast64_t>(std::round(accumulatedMs)));
		}
		return timeStamps;
	}

	void removeMidSpin(const Level& level, std::vector<double>& rotationAngles) {
		const size_t size = rotationAngles.size();
		for (size_t i = size; i > 0; i--) {
			if (level.isMidSpin(i)) {
				rotationAngles.erase(rotationAngles.begin()+i-1);
			}
		}
	}

	void sort(std::vector<InputEvent>& events, std::vector<uint_fast64_t>& timeStamps) {
		if (timeStamps.size() <= 1 || events.size() != timeStamps.size()) return;

		std::vector<size_t> indices(timeStamps.size());
		for (size_t i = 0; i < indices.size(); ++i) {
			indices[i] = i;
		}

		std::sort(indices.begin(), indices.end(), [&timeStamps](size_t a, size_t b) {
			return timeStamps[a] < timeStamps[b];
		});

		std::vector<uint_fast64_t> sortedTimeStamps(timeStamps.size());
		std::vector<InputEvent> sortedEvents(events.size());
		for (size_t i = 0; i < indices.size(); ++i) {
			sortedTimeStamps[i] = timeStamps[indices[i]];
			sortedEvents[i] = events[indices[i]];
		}

		timeStamps = std::move(sortedTimeStamps);
		events = std::move(sortedEvents);
	}

	PlayScript::PlayScript(const Level& level, double maxIndexingBpm) {
		double baseBpm = level.getSetting<double>("bpm");
		while (baseBpm <= 550) baseBpm *= 2;
		while (baseBpm > 1100) baseBpm /= 2;
		if (baseBpm > maxIndexingBpm) baseBpm /= 2;

		std::vector<double> rotationAngles = computeRotationAngles(level);
		processTwirl(level,rotationAngles);
		processPause(level,rotationAngles);
		processSetSpeed(level,rotationAngles,baseBpm);

		// 计算时间戳(会删去开头不用打的轨道的旋转角)
		removeMidSpin(level,rotationAngles);
		floorTimeStamps = computeTimeStamps(rotationAngles,baseBpm);
		// 避免allocateFingers边界检查
		floorTimeStamps.push_back(2*floorTimeStamps[floorTimeStamps.size()-1]-floorTimeStamps[floorTimeStamps.size()-2]);
		rotationAngles.push_back(rotationAngles[rotationAngles.size()-1]);

		size_t size = rotationAngles.size();

		inputs.reserve(size*2);
		timeStamps.reserve(size*2);
		size_t index = 0;
		std::vector<std::pair<int,double>> countsAndDegrees;
		while (index < size) {
			double degree = 0;
			int count = 0;
			do {
				degree += rotationAngles.at(index);
				index++;count++;
			} while (degree < 180 && index < size);
			countsAndDegrees.emplace_back(count,degree);
		}

		size = countsAndDegrees.size();
		countsAndDegrees.emplace_back(std::numeric_limits<int>::max(),std::numeric_limits<double>::max());
		bool usePreferredHand = true;
		for (index = 0; index < size; index++) {
			auto pair = countsAndDegrees[index];
			auto next = countsAndDegrees[index+1];
			bool lasting = false;
			if (pair.second <= 270) {
				lasting = true;
			}
			allocateFingers(pair.first, usePreferredHand, lasting);
			if (pair.second <= 270) {
				usePreferredHand = !usePreferredHand;
			} else if (pair.second <= 450) {
				continue;
			} else if (pair.second <= 630) {
				usePreferredHand = !usePreferredHand;
			} else if (pair.second <= 810) {
				continue;
			} else {
				usePreferredHand = true;
			}
		}
		sort(inputs,timeStamps);
	}

	void PlayScript::allocateFingers(int count, bool usePreferredHand, bool lasting, bool recursion) {
		if (count <= 0) {
			return;
		}
		if (usePreferredHand) {
			if (count == 1) {
				inputs.emplace_back(p1,true);
				inputs.emplace_back(p1,false);
				timeStamps.push_back(floorTimeStamps[allocatingIndex]);
				if (lasting) {
					timeStamps.push_back(floorTimeStamps[allocatingIndex+1]);
				} else {
					timeStamps.push_back((floorTimeStamps[allocatingIndex]+floorTimeStamps[allocatingIndex+1])/2);
				}
			} else if (count == 2) {
				inputs.emplace_back(p2,true);
				inputs.emplace_back(p1,true);
				inputs.emplace_back(p1,false);
				inputs.emplace_back(p2,false);
				timeStamps.push_back(floorTimeStamps[allocatingIndex]);
				timeStamps.push_back(floorTimeStamps[allocatingIndex+1]);
				if (lasting) {
					timeStamps.push_back(floorTimeStamps[allocatingIndex+2]);
				} else {
					timeStamps.push_back(timeStamps[allocatingIndex1+1] + timeStamps[allocatingIndex1+1] - timeStamps[allocatingIndex1+0]);
				}
				timeStamps.push_back(timeStamps[allocatingIndex1+2] + timeStamps[allocatingIndex1+1] - timeStamps[allocatingIndex1+0]);
			} else if (count == 3) {
				inputs.emplace_back(p3,true);
				inputs.emplace_back(p2,true);
				inputs.emplace_back(p1,true);
				inputs.emplace_back(p1,false);
				inputs.emplace_back(p2,false);
				inputs.emplace_back(p3,false);
				timeStamps.push_back(floorTimeStamps[allocatingIndex]);
				timeStamps.push_back(floorTimeStamps[allocatingIndex+1]);
				timeStamps.push_back(floorTimeStamps[allocatingIndex+2]);
				if (lasting) {
					timeStamps.push_back(floorTimeStamps[allocatingIndex+3]);
				} else {

					timeStamps.push_back(timeStamps[allocatingIndex1+2] + timeStamps[allocatingIndex1+2] - timeStamps[allocatingIndex1+1]);
				}
				timeStamps.push_back(timeStamps[allocatingIndex1+3] + timeStamps[allocatingIndex1+2] - timeStamps[allocatingIndex1+1]);
				timeStamps.push_back(timeStamps[allocatingIndex1+4] + timeStamps[allocatingIndex1+1] - timeStamps[allocatingIndex1+0]);
			} else if (count == 4) {
				inputs.emplace_back(p4,true);
				inputs.emplace_back(p3,true);
				inputs.emplace_back(p2,true);
				inputs.emplace_back(p1,true);
				inputs.emplace_back(p1,false);
				inputs.emplace_back(p2,false);
				inputs.emplace_back(p3,false);
				inputs.emplace_back(p4,false);
				timeStamps.push_back(floorTimeStamps[allocatingIndex]);
				timeStamps.push_back(floorTimeStamps[allocatingIndex+1]);
				timeStamps.push_back(floorTimeStamps[allocatingIndex+2]);
				timeStamps.push_back(floorTimeStamps[allocatingIndex+3]);
				if (lasting) {
					timeStamps.push_back(floorTimeStamps[allocatingIndex+4]);
				} else {
					timeStamps.push_back(timeStamps[allocatingIndex1+3] + timeStamps[allocatingIndex1+3] - timeStamps[allocatingIndex1+2]);
				}
				timeStamps.push_back(timeStamps[allocatingIndex1+4] + timeStamps[allocatingIndex1+3] - timeStamps[allocatingIndex1+2]);
				timeStamps.push_back(timeStamps[allocatingIndex1+5] + timeStamps[allocatingIndex1+2] - timeStamps[allocatingIndex1+1]);
				timeStamps.push_back(timeStamps[allocatingIndex1+6] + timeStamps[allocatingIndex1+1] - timeStamps[allocatingIndex1+0]);
			} else if (count == 5) {
				inputs.emplace_back(p4,true);
				inputs.emplace_back(p3,true);
				inputs.emplace_back(p2,true);
				inputs.emplace_back(p1,true);
				inputs.emplace_back(sp1,true);
				inputs.emplace_back(sp1,false);
				inputs.emplace_back(p1,false);
				inputs.emplace_back(p2,false);
				inputs.emplace_back(p3,false);
				inputs.emplace_back(p4,false);
				timeStamps.push_back(floorTimeStamps[allocatingIndex]);
				timeStamps.push_back(floorTimeStamps[allocatingIndex+1]);
				timeStamps.push_back(floorTimeStamps[allocatingIndex+2]);
				timeStamps.push_back(floorTimeStamps[allocatingIndex+3]);
				timeStamps.push_back(floorTimeStamps[allocatingIndex+4]);
				if (lasting) {
					timeStamps.push_back(floorTimeStamps[allocatingIndex+5]);
				} else {
					timeStamps.push_back(timeStamps[allocatingIndex1+4] + timeStamps[allocatingIndex1+4] - timeStamps[allocatingIndex1+3]);
				}
				timeStamps.push_back(timeStamps[allocatingIndex1+5] + timeStamps[allocatingIndex1+4] - timeStamps[allocatingIndex1+3]);
				timeStamps.push_back(timeStamps[allocatingIndex1+6] + timeStamps[allocatingIndex1+3] - timeStamps[allocatingIndex1+2]);
				timeStamps.push_back(timeStamps[allocatingIndex1+7] + timeStamps[allocatingIndex1+2] - timeStamps[allocatingIndex1+1]);
				timeStamps.push_back(timeStamps[allocatingIndex1+8] + timeStamps[allocatingIndex1+1] - timeStamps[allocatingIndex1+0]);
			} else if (count == 6) {
				inputs.emplace_back(p4,true);
				inputs.emplace_back(p3,true);
				inputs.emplace_back(p2,true);
				inputs.emplace_back(sp2,true);
				inputs.emplace_back(p1,true);
				inputs.emplace_back(sp1,true);
				inputs.emplace_back(sp1,false);
				inputs.emplace_back(p1,false);
				inputs.emplace_back(sp2,false);
				inputs.emplace_back(p2,false);
				inputs.emplace_back(p3,false);
				inputs.emplace_back(p4,false);
				timeStamps.push_back(floorTimeStamps[allocatingIndex]);
				timeStamps.push_back(floorTimeStamps[allocatingIndex+1]);
				timeStamps.push_back(floorTimeStamps[allocatingIndex+2]);
				timeStamps.push_back(floorTimeStamps[allocatingIndex+3]);
				timeStamps.push_back(floorTimeStamps[allocatingIndex+4]);
				timeStamps.push_back(floorTimeStamps[allocatingIndex+5]);
				if (lasting) {
					timeStamps.push_back(floorTimeStamps[allocatingIndex+6]);
				} else {
					timeStamps.push_back(timeStamps[allocatingIndex1+5] + timeStamps[allocatingIndex1+5] - timeStamps[allocatingIndex1+4]);
				}
				timeStamps.push_back(timeStamps[allocatingIndex1+6] + timeStamps[allocatingIndex1+5] - timeStamps[allocatingIndex1+4]);
				timeStamps.push_back(0);
				timeStamps.push_back(timeStamps[allocatingIndex1+7] + timeStamps[allocatingIndex1+4] - timeStamps[allocatingIndex1+2]);
				timeStamps[allocatingIndex1+8] = timeStamps[allocatingIndex1+9] - timeStamps[allocatingIndex1+3] + timeStamps[allocatingIndex1+2];
				timeStamps.push_back(timeStamps[allocatingIndex1+9] + timeStamps[allocatingIndex1+2] - timeStamps[allocatingIndex1+1]);
				timeStamps.push_back(timeStamps[allocatingIndex1+10] + timeStamps[allocatingIndex1+1] - timeStamps[allocatingIndex1+0]);
			} else if (count == 7) {
				inputs.emplace_back(p4,true);
				inputs.emplace_back(sp4,true);
				inputs.emplace_back(p3,true);
				inputs.emplace_back(p2,true);
				inputs.emplace_back(sp2,true);
				inputs.emplace_back(p1,true);
				inputs.emplace_back(sp1,true);
				inputs.emplace_back(sp1,false);
				inputs.emplace_back(p1,false);
				inputs.emplace_back(sp2,false);
				inputs.emplace_back(p2,false);
				inputs.emplace_back(p3,false);
				inputs.emplace_back(sp4,false);
				inputs.emplace_back(p4,false);
				timeStamps.push_back(floorTimeStamps[allocatingIndex]);
				timeStamps.push_back(floorTimeStamps[allocatingIndex+1]);
				timeStamps.push_back(floorTimeStamps[allocatingIndex+2]);
				timeStamps.push_back(floorTimeStamps[allocatingIndex+3]);
				timeStamps.push_back(floorTimeStamps[allocatingIndex+4]);
				timeStamps.push_back(floorTimeStamps[allocatingIndex+5]);
				timeStamps.push_back(floorTimeStamps[allocatingIndex+6]);
				if (lasting) {
					timeStamps.push_back(floorTimeStamps[allocatingIndex+7]);
				} else {
					timeStamps.push_back(timeStamps[allocatingIndex1+6] + timeStamps[allocatingIndex1+6] - timeStamps[allocatingIndex1+5]);
				}
				timeStamps.push_back(timeStamps[allocatingIndex1+7] + timeStamps[allocatingIndex1+6] - timeStamps[allocatingIndex1+5]);
				timeStamps.push_back(0);
				timeStamps.push_back(timeStamps[allocatingIndex1+8] + timeStamps[allocatingIndex1+5] - timeStamps[allocatingIndex1+3]);
				timeStamps[allocatingIndex1+9] = timeStamps[allocatingIndex1+10] - timeStamps[allocatingIndex1+4] + timeStamps[allocatingIndex1+3];
				timeStamps.push_back(timeStamps[allocatingIndex1+10] + timeStamps[allocatingIndex1+3] - timeStamps[allocatingIndex1+2]);
				timeStamps.push_back(0);
				timeStamps.push_back(timeStamps[allocatingIndex1+11] + timeStamps[allocatingIndex1+2] - timeStamps[allocatingIndex1+0]);
				timeStamps[allocatingIndex1+12] = timeStamps[allocatingIndex1+13] - timeStamps[allocatingIndex1+1] + timeStamps[allocatingIndex1+0];
			} else if (count == 8) {
				inputs.emplace_back(p4,true);
				inputs.emplace_back(sp4,true);
				inputs.emplace_back(p3,true);
				inputs.emplace_back(sp3,true);
				inputs.emplace_back(p2,true);
				inputs.emplace_back(sp2,true);
				inputs.emplace_back(p1,true);
				inputs.emplace_back(sp1,true);
				inputs.emplace_back(sp1,false);
				inputs.emplace_back(p1,false);
				inputs.emplace_back(sp2,false);
				inputs.emplace_back(p2,false);
				inputs.emplace_back(sp3,false);
				inputs.emplace_back(p3,false);
				inputs.emplace_back(sp4,false);
				inputs.emplace_back(p4,false);
				timeStamps.push_back(floorTimeStamps[allocatingIndex]);
				timeStamps.push_back(floorTimeStamps[allocatingIndex+1]);
				timeStamps.push_back(floorTimeStamps[allocatingIndex+2]);
				timeStamps.push_back(floorTimeStamps[allocatingIndex+3]);
				timeStamps.push_back(floorTimeStamps[allocatingIndex+4]);
				timeStamps.push_back(floorTimeStamps[allocatingIndex+5]);
				timeStamps.push_back(floorTimeStamps[allocatingIndex+6]);
				timeStamps.push_back(floorTimeStamps[allocatingIndex+7]);
				if (lasting) {
					timeStamps.push_back(floorTimeStamps[allocatingIndex+8]);
				} else {
					timeStamps.push_back(timeStamps[allocatingIndex1+7] + timeStamps[allocatingIndex1+7] - timeStamps[allocatingIndex1+6]);
				}
				timeStamps.push_back(timeStamps[allocatingIndex1+8] + timeStamps[allocatingIndex1+7] - timeStamps[allocatingIndex1+6]);
				timeStamps.push_back(0);
				timeStamps.push_back(timeStamps[allocatingIndex1+9] + timeStamps[allocatingIndex1+6] - timeStamps[allocatingIndex1+4]);
				timeStamps[allocatingIndex1+10] = timeStamps[allocatingIndex1+11] - timeStamps[allocatingIndex1+5] + timeStamps[allocatingIndex1+4];
				timeStamps.push_back(0);
				timeStamps.push_back(timeStamps[allocatingIndex1+11] + timeStamps[allocatingIndex1+4] - timeStamps[allocatingIndex1+2]);
				timeStamps[allocatingIndex1+12] = timeStamps[allocatingIndex1+13] - timeStamps[allocatingIndex1+3] + timeStamps[allocatingIndex1+2];
				timeStamps.push_back(0);
				timeStamps.push_back(timeStamps[allocatingIndex1+13] + timeStamps[allocatingIndex1+2] - timeStamps[allocatingIndex1+0]);
				timeStamps[allocatingIndex1+14] = timeStamps[allocatingIndex1+15] - timeStamps[allocatingIndex1+1] + timeStamps[allocatingIndex1+0];
			} else {
				allocatingIndex -= count;
				allocatingIndex1 -= count*2;
				int thisHandKeyCount = count / 2;
				int nextHandKeyCount = count - thisHandKeyCount;
				allocateFingers(thisHandKeyCount, true, lasting,true);
				allocateFingers(nextHandKeyCount, false, lasting,true);
			}
		} else {
			if (count == 1) {
				inputs.emplace_back(n1,true);
				inputs.emplace_back(n1,false);
				timeStamps.push_back(floorTimeStamps[allocatingIndex]);
				if (lasting) {
					timeStamps.push_back(floorTimeStamps[allocatingIndex+1]);
				} else {
					timeStamps.push_back((floorTimeStamps[allocatingIndex]+floorTimeStamps[allocatingIndex+1])/2);
				}
			} else if (count == 2) {
				inputs.emplace_back(n2,true);
				inputs.emplace_back(n1,true);
				inputs.emplace_back(n1,false);
				inputs.emplace_back(n2,false);
				timeStamps.push_back(floorTimeStamps[allocatingIndex]);
				timeStamps.push_back(floorTimeStamps[allocatingIndex+1]);
				if (lasting) {
					timeStamps.push_back(floorTimeStamps[allocatingIndex+2]);
				} else {
					timeStamps.push_back(timeStamps[allocatingIndex1+1] + timeStamps[allocatingIndex1+1] - timeStamps[allocatingIndex1+0]);
				}
				timeStamps.push_back(timeStamps[allocatingIndex1+2] + timeStamps[allocatingIndex1+1] - timeStamps[allocatingIndex1+0]);
			} else if (count == 3) {
				inputs.emplace_back(n3,true);
				inputs.emplace_back(n2,true);
				inputs.emplace_back(n1,true);
				inputs.emplace_back(n1,false);
				inputs.emplace_back(n2,false);
				inputs.emplace_back(n3,false);
				timeStamps.push_back(floorTimeStamps[allocatingIndex]);
				timeStamps.push_back(floorTimeStamps[allocatingIndex+1]);
				timeStamps.push_back(floorTimeStamps[allocatingIndex+2]);
				if (lasting) {
					timeStamps.push_back(floorTimeStamps[allocatingIndex+3]);
				} else {

					timeStamps.push_back(timeStamps[allocatingIndex1+2] + timeStamps[allocatingIndex1+2] - timeStamps[allocatingIndex1+1]);
				}
				timeStamps.push_back(timeStamps[allocatingIndex1+3] + timeStamps[allocatingIndex1+2] - timeStamps[allocatingIndex1+1]);
				timeStamps.push_back(timeStamps[allocatingIndex1+4] + timeStamps[allocatingIndex1+1] - timeStamps[allocatingIndex1+0]);
			} else if (count == 4) {
				inputs.emplace_back(n4,true);
				inputs.emplace_back(n3,true);
				inputs.emplace_back(n2,true);
				inputs.emplace_back(n1,true);
				inputs.emplace_back(n1,false);
				inputs.emplace_back(n2,false);
				inputs.emplace_back(n3,false);
				inputs.emplace_back(n4,false);
				timeStamps.push_back(floorTimeStamps[allocatingIndex]);
				timeStamps.push_back(floorTimeStamps[allocatingIndex+1]);
				timeStamps.push_back(floorTimeStamps[allocatingIndex+2]);
				timeStamps.push_back(floorTimeStamps[allocatingIndex+3]);
				if (lasting) {
					timeStamps.push_back(floorTimeStamps[allocatingIndex+4]);
				} else {
					timeStamps.push_back(timeStamps[allocatingIndex1+3] + timeStamps[allocatingIndex1+3] - timeStamps[allocatingIndex1+2]);
				}
				timeStamps.push_back(timeStamps[allocatingIndex1+4] + timeStamps[allocatingIndex1+3] - timeStamps[allocatingIndex1+2]);
				timeStamps.push_back(timeStamps[allocatingIndex1+5] + timeStamps[allocatingIndex1+2] - timeStamps[allocatingIndex1+1]);
				timeStamps.push_back(timeStamps[allocatingIndex1+6] + timeStamps[allocatingIndex1+1] - timeStamps[allocatingIndex1+0]);
			} else if (count == 5) {
				inputs.emplace_back(n4,true);
				inputs.emplace_back(n3,true);
				inputs.emplace_back(n2,true);
				inputs.emplace_back(n1,true);
				inputs.emplace_back(sn1,true);
				inputs.emplace_back(sn1,false);
				inputs.emplace_back(n1,false);
				inputs.emplace_back(n2,false);
				inputs.emplace_back(n3,false);
				inputs.emplace_back(n4,false);
				timeStamps.push_back(floorTimeStamps[allocatingIndex]);
				timeStamps.push_back(floorTimeStamps[allocatingIndex+1]);
				timeStamps.push_back(floorTimeStamps[allocatingIndex+2]);
				timeStamps.push_back(floorTimeStamps[allocatingIndex+3]);
				timeStamps.push_back(floorTimeStamps[allocatingIndex+4]);
				if (lasting) {
					timeStamps.push_back(floorTimeStamps[allocatingIndex+5]);
				} else {
					timeStamps.push_back(timeStamps[allocatingIndex1+4] + timeStamps[allocatingIndex1+4] - timeStamps[allocatingIndex1+3]);
				}
				timeStamps.push_back(timeStamps[allocatingIndex1+5] + timeStamps[allocatingIndex1+4] - timeStamps[allocatingIndex1+3]);
				timeStamps.push_back(timeStamps[allocatingIndex1+6] + timeStamps[allocatingIndex1+3] - timeStamps[allocatingIndex1+2]);
				timeStamps.push_back(timeStamps[allocatingIndex1+7] + timeStamps[allocatingIndex1+2] - timeStamps[allocatingIndex1+1]);
				timeStamps.push_back(timeStamps[allocatingIndex1+8] + timeStamps[allocatingIndex1+1] - timeStamps[allocatingIndex1+0]);
			} else if (count == 6) {
				inputs.emplace_back(n4,true);
				inputs.emplace_back(n3,true);
				inputs.emplace_back(n2,true);
				inputs.emplace_back(sn2,true);
				inputs.emplace_back(n1,true);
				inputs.emplace_back(sn1,true);
				inputs.emplace_back(sn1,false);
				inputs.emplace_back(n1,false);
				inputs.emplace_back(sn2,false);
				inputs.emplace_back(n2,false);
				inputs.emplace_back(n3,false);
				inputs.emplace_back(n4,false);
				timeStamps.push_back(floorTimeStamps[allocatingIndex]);
				timeStamps.push_back(floorTimeStamps[allocatingIndex+1]);
				timeStamps.push_back(floorTimeStamps[allocatingIndex+2]);
				timeStamps.push_back(floorTimeStamps[allocatingIndex+3]);
				timeStamps.push_back(floorTimeStamps[allocatingIndex+4]);
				timeStamps.push_back(floorTimeStamps[allocatingIndex+5]);
				if (lasting) {
					timeStamps.push_back(floorTimeStamps[allocatingIndex+6]);
				} else {
					timeStamps.push_back(timeStamps[allocatingIndex1+5] + timeStamps[allocatingIndex1+5] - timeStamps[allocatingIndex1+4]);
				}
				timeStamps.push_back(timeStamps[allocatingIndex1+6] + timeStamps[allocatingIndex1+5] - timeStamps[allocatingIndex1+4]);
				timeStamps.push_back(0);
				timeStamps.push_back(timeStamps[allocatingIndex1+7] + timeStamps[allocatingIndex1+4] - timeStamps[allocatingIndex1+2]);
				timeStamps[allocatingIndex1+8] = timeStamps[allocatingIndex1+9] - timeStamps[allocatingIndex1+3] + timeStamps[allocatingIndex1+2];
				timeStamps.push_back(timeStamps[allocatingIndex1+9] + timeStamps[allocatingIndex1+2] - timeStamps[allocatingIndex1+1]);
				timeStamps.push_back(timeStamps[allocatingIndex1+10] + timeStamps[allocatingIndex1+1] - timeStamps[allocatingIndex1+0]);
			} else if (count == 7) {
				inputs.emplace_back(n4,true);
				inputs.emplace_back(sn4,true);
				inputs.emplace_back(n3,true);
				inputs.emplace_back(n2,true);
				inputs.emplace_back(sn2,true);
				inputs.emplace_back(n1,true);
				inputs.emplace_back(sn1,true);
				inputs.emplace_back(sn1,false);
				inputs.emplace_back(n1,false);
				inputs.emplace_back(sn2,false);
				inputs.emplace_back(n2,false);
				inputs.emplace_back(n3,false);
				inputs.emplace_back(sn4,false);
				inputs.emplace_back(n4,false);
				timeStamps.push_back(floorTimeStamps[allocatingIndex]);
				timeStamps.push_back(floorTimeStamps[allocatingIndex+1]);
				timeStamps.push_back(floorTimeStamps[allocatingIndex+2]);
				timeStamps.push_back(floorTimeStamps[allocatingIndex+3]);
				timeStamps.push_back(floorTimeStamps[allocatingIndex+4]);
				timeStamps.push_back(floorTimeStamps[allocatingIndex+5]);
				timeStamps.push_back(floorTimeStamps[allocatingIndex+6]);
				if (lasting) {
					timeStamps.push_back(floorTimeStamps[allocatingIndex+7]);
				} else {
					timeStamps.push_back(timeStamps[allocatingIndex1+6] + timeStamps[allocatingIndex1+6] - timeStamps[allocatingIndex1+5]);
				}
				timeStamps.push_back(timeStamps[allocatingIndex1+7] + timeStamps[allocatingIndex1+6] - timeStamps[allocatingIndex1+5]);
				timeStamps.push_back(0);
				timeStamps.push_back(timeStamps[allocatingIndex1+8] + timeStamps[allocatingIndex1+5] - timeStamps[allocatingIndex1+3]);
				timeStamps[allocatingIndex1+9] = timeStamps[allocatingIndex1+10] - timeStamps[allocatingIndex1+4] + timeStamps[allocatingIndex1+3];
				timeStamps.push_back(timeStamps[allocatingIndex1+10] + timeStamps[allocatingIndex1+3] - timeStamps[allocatingIndex1+2]);
				timeStamps.push_back(0);
				timeStamps.push_back(timeStamps[allocatingIndex1+11] + timeStamps[allocatingIndex1+2] - timeStamps[allocatingIndex1+0]);
				timeStamps[allocatingIndex1+12] = timeStamps[allocatingIndex1+13] - timeStamps[allocatingIndex1+1] + timeStamps[allocatingIndex1+0];
			} else if (count == 8) {
				inputs.emplace_back(n4,true);
				inputs.emplace_back(sn4,true);
				inputs.emplace_back(n3,true);
				inputs.emplace_back(sn3,true);
				inputs.emplace_back(n2,true);
				inputs.emplace_back(sn2,true);
				inputs.emplace_back(n1,true);
				inputs.emplace_back(sn1,true);
				inputs.emplace_back(sn1,false);
				inputs.emplace_back(n1,false);
				inputs.emplace_back(sn2,false);
				inputs.emplace_back(n2,false);
				inputs.emplace_back(sn3,false);
				inputs.emplace_back(n3,false);
				inputs.emplace_back(sn4,false);
				inputs.emplace_back(n4,false);
				timeStamps.push_back(floorTimeStamps[allocatingIndex]);
				timeStamps.push_back(floorTimeStamps[allocatingIndex+1]);
				timeStamps.push_back(floorTimeStamps[allocatingIndex+2]);
				timeStamps.push_back(floorTimeStamps[allocatingIndex+3]);
				timeStamps.push_back(floorTimeStamps[allocatingIndex+4]);
				timeStamps.push_back(floorTimeStamps[allocatingIndex+5]);
				timeStamps.push_back(floorTimeStamps[allocatingIndex+6]);
				timeStamps.push_back(floorTimeStamps[allocatingIndex+7]);
				if (lasting) {
					timeStamps.push_back(floorTimeStamps[allocatingIndex+8]);
				} else {
					timeStamps.push_back(timeStamps[allocatingIndex1+7] + timeStamps[allocatingIndex1+7] - timeStamps[allocatingIndex1+6]);
				}
				timeStamps.push_back(timeStamps[allocatingIndex1+8] + timeStamps[allocatingIndex1+7] - timeStamps[allocatingIndex1+6]);
				timeStamps.push_back(0);
				timeStamps.push_back(timeStamps[allocatingIndex1+9] + timeStamps[allocatingIndex1+6] - timeStamps[allocatingIndex1+4]);
				timeStamps[allocatingIndex1+10] = timeStamps[allocatingIndex1+11] - timeStamps[allocatingIndex1+5] + timeStamps[allocatingIndex1+4];
				timeStamps.push_back(0);
				timeStamps.push_back(timeStamps[allocatingIndex1+11] + timeStamps[allocatingIndex1+4] - timeStamps[allocatingIndex1+2]);
				timeStamps[allocatingIndex1+12] = timeStamps[allocatingIndex1+13] - timeStamps[allocatingIndex1+3] + timeStamps[allocatingIndex1+2];
				timeStamps.push_back(0);
				timeStamps.push_back(timeStamps[allocatingIndex1+13] + timeStamps[allocatingIndex1+2] - timeStamps[allocatingIndex1+0]);
				timeStamps[allocatingIndex1+14] = timeStamps[allocatingIndex1+15] - timeStamps[allocatingIndex1+1] + timeStamps[allocatingIndex1+0];
			} else {
				allocatingIndex -= count;
				allocatingIndex1 -= count*2;
				int thisHandKeyCount = count / 2;
				int nextHandKeyCount = count - thisHandKeyCount;
				allocateFingers(thisHandKeyCount, false,  lasting,true);
				allocateFingers(nextHandKeyCount, true,  lasting,true);
			}
		}
		allocatingIndex += count;
		allocatingIndex1 += count*2;
	}

}