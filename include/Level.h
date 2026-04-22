#pragma once

#include <PlayScript.h>
#include <Range.h>
#include <vector>
#include <nlohmann/json.hpp>

namespace ReADOFAIMacro {

	class PlayScript;

	class Level {
	public:
		explicit Level(const std::string& path);
		// angleData不存储出发点数据(默认0)，为了对齐floor, angleData, rotationAngle数据推荐使用：
		double angleAtFloor(const size_t& floor) const {
			if (floor == 0)
				return 0;
			return angleData.at(floor - 1);
		}
		// 轨道总数(含出发点)
		size_t getFloorCount() const {
			return angleData.size() + 1;
		}
		// 判断floor位置的轨道是不是中旋(999)
		bool isMidSpin(const size_t& floor) const {
			return angleAtFloor(floor) == 999;
		}

		auto getAngleData() const -> const std::vector<double>& {
			return angleData;
		}
		auto getSettings() const -> const nlohmann::json& {
			return settings;
		}
		template <typename T> T getSetting(const std::string& key) const {
			return settings.at(key).get<T>();
		}
		auto getActions() const -> const std::vector<nlohmann::json>& {
			return actions;
		}
		auto getDecorations() const -> const std::vector<nlohmann::json>& {
			return decorations;
		}
		auto getSpeedList() const -> const std::vector<std::pair<Range, std::pair<double,double>>>& {
			return speedList;
		}
		auto getTwirlRanges() const -> const std::vector<Range>& {
			return twirlRanges;
		}
		auto getThreePlanetsRanges() const -> const std::vector<Range>& {
			return threePlanetsRanges;
		}
		auto getPauses() const -> const std::vector<std::pair<size_t, double>>& {
			return pauses;
		}
	private:
		std::vector<double> angleData;
		nlohmann::json settings;
		std::vector<nlohmann::json> actions;
		std::vector<nlohmann::json> decorations;

		std::vector<uint_fast64_t> floorTimeStamps;
		std::vector<std::pair<Range, std::pair<double,double>>> speedList; // 设置速度的数据，结构为vector<pair<使用这个速度的轨道, <BPM, 倍频器>>>
		std::vector<Range> twirlRanges;
		std::vector<Range> threePlanetsRanges;
		std::vector<std::pair<size_t, double>> pauses;
	};

} // namespace ReADOFAIMacro
