#include <level.h>

#include <fstream>
#include <regex>

// 发卡弯bug: 三球时发卡弯上的暂停节拍事件比实际暂停少1拍
void sb7bg(double& duration) {
	duration += 1;
}
auto pathDataToAngleData(const std::string& pathData) -> std::vector<double> {
	const std::unordered_map<char, int> MAPPING = {
		{'R', 0},	{'p', 15},	{'J', 30},	{'E', 45},	{'T', 60},	{'o', 75},	{'U', 90},
		{'q', 105}, {'G', 120}, {'Q', 135}, {'H', 150}, {'W', 165}, {'L', 180}, {'x', 195},
		{'N', 210}, {'Z', 225}, {'F', 240}, {'V', 255}, {'D', 270}, {'Y', 285}, {'B', 300},
		{'C', 315}, {'M', 330}, {'A', 345}, {'!', 999}};
	std::vector<double> result;
	result.reserve(pathData.size());
	for (char ch : pathData) {
		result.push_back(MAPPING.at(ch));
	}
	return result;
}

namespace ReADOFAIMacro {

	Level::Level(const std::string& path) {
		// 将关卡文件数据加载到angleData,settings,actions,decorations字段中
		{
			std::ifstream file(path);
			std::stringstream buffer;
			buffer << file.rdbuf();
			std::string jsonString = buffer.str();

			jsonString = std::regex_replace(jsonString, std::regex("\n"),"");
			jsonString = std::regex_replace(jsonString, std::regex(", *}"),"}");

			nlohmann::json json = nlohmann::json::parse(jsonString);
			file.close();

			if (json.contains("angleData")) {
				json.at("angleData").get_to(angleData);
			} else if (json.contains("pathData")) {
				const std::string pathData = json.at("pathData").get<std::string>();
				angleData = pathDataToAngleData(pathData);
			} else {
				throw std::runtime_error("Level missing angleData or pathData");
			}
			if (json.contains("settings")) {
				json.at("settings").get_to(settings);
			} else {
				throw std::runtime_error("Level missing settings");
			}
			if (json.contains("actions")) {
				json.at("actions").get_to(actions);
			} else {
				throw std::runtime_error("Level missing actions");
			}
			if (json.contains("decorations")) {
				json.at("decorations").get_to(decorations);
			}
		}
		// 结构化事件数据
		{
			const size_t actionsSize = actions.size();
			bool isThreePlanets = false;
			size_t lastSetSpeedFloor = 0;
			double lastBpm = getSetting<double>("bpm");
			size_t lastThreePlanetsFloor = 0;
			size_t lastTwoPlanetsFloor = 0;
			double multiplier = 1;
			size_t lastTwirlFloor = 0;
			bool isTwirled = false;

			for (size_t i = 0; i < actionsSize; i++) {
				const nlohmann::json& action = actions.at(i);
				const auto eventType = action.at("eventType").get<std::string>();
				const int floor = action.at("floor").get<int>();
				if (eventType == "SetSpeed")  {
					const std::string& speedType = action.at("speedType").get<std::string>();
					speedList.emplace_back(Range(lastSetSpeedFloor, floor),
										   std::pair<double,double>(lastBpm, multiplier));
					if (speedType == "Multiplier") {
						multiplier = action.at("bpmMultiplier").get<double>();
						lastBpm *= multiplier;
					} else if (speedType == "Bpm") {
						const double& bpm = action.at("beatsPerMinute").get<double>();
						multiplier = bpm / lastBpm;
						lastBpm = bpm;
					} else {
						throw std::runtime_error("Unknown speed type: " + speedType);
					}
					lastSetSpeedFloor = floor;
				} else if (eventType == "Twirl") {
					if (isTwirled) {
						twirlRanges.emplace_back(lastTwirlFloor, floor);
					}
					isTwirled = !isTwirled;
					lastTwirlFloor = floor;
				} else if (eventType == "Pause") {
					double duration = action.at("duration").get<double>();
					if (angleAtFloor(floor) == 360 && isThreePlanets) {
						sb7bg(duration);
					}
					pauses.emplace_back(floor, duration);
				} else if (eventType == "MultiPlanet") {
					isThreePlanets = action.at("planets").get<std::string>() == "ThreePlanets";
					if (isThreePlanets) {
						if (lastThreePlanetsFloor < lastTwoPlanetsFloor) {
							lastThreePlanetsFloor = floor;
						}
					}
					else {
						if (lastThreePlanetsFloor > lastTwoPlanetsFloor) {
							lastTwoPlanetsFloor = floor;
							threePlanetsRanges.emplace_back(lastThreePlanetsFloor, floor);
						}
					}
				}
			}

			const size_t lastFloor = getFloorCount()-1;
			speedList.emplace_back(Range(lastSetSpeedFloor, lastFloor),
								   std::pair<double,double>(lastBpm, multiplier));
			if (isTwirled)
				twirlRanges.emplace_back(lastTwirlFloor, lastFloor);
			if (isThreePlanets)
				threePlanetsRanges.emplace_back(lastThreePlanetsFloor, lastFloor);
		}
	}

}