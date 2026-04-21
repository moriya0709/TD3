#pragma once
#include<string>
#include<vector>
#include <fstream>
#include<externals//nlohmann/json.hpp>

using json = nlohmann::json;

//保存したいデータ構造
struct ScoreData
{
	int score;
	std::string stageName;
	std::string model;
	float time;
};

//JSON変換定義
inline void to_json(json& j, const ScoreData& s)
{
	j = json{
		{"score",s.score},
		{"stageName",s.stageName},
		{"ModelName",s.model},
		{"time",s.time}
	};
}

inline void from_json(const json& j, ScoreData& s)
{
	j.at("score").get_to(s.score);
	j.at("stageName").get_to(s.stageName);
	j.at("ModelName").get_to(s.model);
	j.at("time").get_to(s.time);
}

class ScoreManager
{
	public:
		//スコアを保存する関数
		void SaveScene(int score, const std::string& stage,const std::string& model,float time)
		{
			ScoreData newRecord = { score,stage,model,time };

			//過去のデータを読み込んでから、新しいデータを追加して保存する
			std::vector<ScoreData>history = LoadHistory();
			history.push_back(newRecord);

			json j;
			j["scores"] = history;

			std::ofstream file("Resource/Data/score_history.json");
			if (file.is_open())
			{
				file << j.dump(4);
			}
		}

		//過去のスコア一覧を読み込む関数
		std::vector<ScoreData> LoadHistory()
		{
			std::vector<ScoreData>history;
			std::ifstream file("Resource/Data/score_history.json");
			if (file.is_open())
			{
				json j;
				file >> j;
				history = j.at("scores").get<std::vector<ScoreData>>();
			}
		return history;
		}
};