#ifndef CLOUD_GAME_API_H_
#define CLOUD_GAME_API_H_

#include<string>
#include <stdio.h>
#include<Windows.h>
#include<codecvt>
#include<locale>
#include"http_client.h"

class ServerResponseListener {
public:
	virtual void onSuccess(std::string response) = 0;
	virtual void onFailed(std::string msg) = 0;
};

class CloudGameApi {
public:
	CloudGameApi();
	~CloudGameApi();

	void startGame(std::string params, ServerResponseListener* listener);
	void stopGame(std::string &params, ServerResponseListener* listener);
	std::string createGameStartParam(const std::string &clientSession, const std::string& experienceCode);

private:
	std::wstring to_wstring(std::string input);
	std::unique_ptr<HttpClient> http_client_;
};

#endif