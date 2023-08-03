#include "cloud_game_api.h"

#include<string>
#include <random>
#include <sstream>
#include <iostream>
#include <thread>

#include "json/json.h"

using namespace std;
namespace uuid {
    static std::random_device              rd;
    static std::mt19937                    gen(rd());
    static std::uniform_int_distribution<> dis(0, 15);
    static std::uniform_int_distribution<> dis2(8, 11);

    std::string generate_uuid_v4() {
        std::stringstream ss;
        int i;
        ss << std::hex;
        for (i = 0; i < 8; i++) {
            ss << dis(gen);
        }
        ss << "-";
        for (i = 0; i < 4; i++) {
            ss << dis(gen);
        }
        ss << "-4";
        for (i = 0; i < 3; i++) {
            ss << dis(gen);
        }
        ss << "-";
        ss << dis2(gen);
        for (i = 0; i < 3; i++) {
            ss << dis(gen);
        }
        ss << "-";
        for (i = 0; i < 12; i++) {
            ss << dis(gen);
        };
        return ss.str();
    }
}

CloudGameApi::CloudGameApi()
{
    http_client_.reset(new HttpClient(L"User-Agent"));
}

CloudGameApi::~CloudGameApi() {

}

void CloudGameApi::startGame(std::string params, ServerResponseListener* listener)
{
    std::thread t([listener, params, this]() {
        std::vector<std::wstring> headers;
        headers.push_back(L"Content-Type: application/json; charset=utf-8\\");

        std::string respData;
        std::wstring url = to_wstring("https://code.cloud-gaming.myqcloud.com/CreateExperienceSession");
        DWORD ret = http_client_->http_post(url, headers, params, respData);
        if (0 != ret || true == respData.empty())
        {
            //请求失败,请检查参数或网络。
            listener->onFailed("Connect to server failed.");
        }
        else {
            listener->onSuccess(respData);
        }
        });
    t.detach();
}

void CloudGameApi::stopGame(std::string& param, ServerResponseListener* listener)
{
}

std::string CloudGameApi::createGameStartParam(const std::string& clientSession, const std::string &experienceCode)
{
    Json::Value root;
    root["RequestId"] = uuid::generate_uuid_v4();
    root["ExperienceCode"] = experienceCode;
    root["UserId"] = uuid::generate_uuid_v4();
    root["ClientSession"] = clientSession;
	return Json::FastWriter().write(root);
}

std::wstring CloudGameApi::to_wstring(std::string input) {
    std::wstring_convert<std::codecvt_utf8<wchar_t>> converter;
    return converter.from_bytes(input);
}