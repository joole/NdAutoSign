#include "NdTask.h"
#include <iostream>
#include <string>
#include <jsoncpp/json/json.h>
#include <jsoncpp/json/reader.h>
#include <jsoncpp/json/value.h>
#include "NdHttpClient.h"
#include "NdCalculateUtil.h"
#include <sstream>
#include <string.h>
#include <cstdio>
#include <chrono>
#include "NdCodeConvert.h"

bool NdTask::operator()(NdUserInfo& user)
{
	do 
	{
		auto begin = std::chrono::high_resolution_clock::now();
		NdUCToken token;
		std::vector<NdBlessInfo> blessArray;
		std::vector<NdExtraPointInfo> extraPointArray;
		
		if (!t_LoginIOA(&user)){
			std::printf("[%s] Failed to Login IOA.\n", user.m_uname.c_str());
			break;
		}

		// 获取UC Token
		std::string encPwd = NdCalculateUtil::MD5Encrypt(user.m_99upwd);
		if (!t_RequestForUC(user.m_uname, encPwd, &token)) {
			std::printf("[%s] Failed to Request UCToken.\n", user.m_uname.c_str());
			break;
		}

		if (user.m_bAutoSign) {
			if (!t_AutoSign(&user))
			{
				std::printf("[%s] Failed To Signed Today.\n", user.m_uname.c_str());
			}
		}
		if (user.m_bBirth) {
			if (blessArray.size() == 0)
			{
				bool retry		= false;
				bool result		= false;
				int retry_count = 0;
				for (auto iCount = 0; iCount < 4; iCount++)
				{
					if (retry_count >= 3) {
						std::printf("[%s] Failed To Get Bless List Using Using Max Retries Count.\n", user.m_uname.c_str());
						break;
					}

					result = t_GetBlessList(&token, &blessArray, retry);
					if (result)
					{
						break;
					}
					else if (result == false && retry == true)
					{
						retry_count++;
						continue;
					}
					else
					{
						std::printf("[%s] Failed To Get Bless List.\n", user.m_uname.c_str());
						break;
					}
				}
			}

			if (!t_BlessBirth(&token, blessArray))
			{
				std::printf("[%s] Failed To Bless Today.\n", user.m_uname.c_str());
			}
			
		}
		if (user.m_bSendFlower) {
			bool send_ok = true;
			for (auto & recv : user.m_sendFlower)
			{
				bool retry = false;
				bool result = false;
				for (;;)
				{
					result = t_SendFlower(&token, recv.m_recverId, recv.m_count, retry);
					if (result == true) {
						send_ok = true;
						break;
					}
					else if (result == false && retry == true) {
						continue;
					}
					else {
						std::printf("[%s] Send %d Flower(s) To [%s] Failed.\n", user.m_uname.c_str(), recv.m_count, recv.m_recverId.c_str());
						break;
					}
				}
			}
		}
		
		if (user.m_bExtraPt) {
			if (!t_GetExtraList(&user, &extraPointArray))
			{
				std::printf("[%s] Failed To Get Extra Points List.\n", user.m_uname.c_str());
				break;
			}
			if (!t_GetExtraPoint(&user, extraPointArray))
			{
				std::printf("[%s] Failed To Get Extra Points.\n", user.m_uname.c_str());
				break;
			}
		}
		// 针对P6 做出改变
		if (user.m_bTaskClear) {
			if (!t_DayTaskClear(&user))
			{
				std::printf("[%s] Failed To Clean Day Task.\n", user.m_uname.c_str());
			}
		}
		if (user.m_bWriteDiary) {
			if (!t_WriteDiray(&user)) {
				std::printf("[%s] Failed To Get Extra Points.\n", user.m_uname.c_str());
				break;
			}
		}
		
		auto end = std::chrono::high_resolution_clock::now();
		std::printf("[%s] Done Job Successfully!!!  using %lld ms.\n", user.m_uname.c_str(), std::chrono::duration_cast<std::chrono::milliseconds>(end - begin).count());
		return true;
		
	} while (false);
	return false;
}

bool NdTask::InitUsers(const char* config_path, std::vector<NdUserInfo> *pUsers)
{
	if (!pUsers)
		return false;
	Json::Reader reader;
	Json::Value root;
	std::string content;
	char buffer[256] = {0};
	FILE* pFile = fopen(config_path, "rb");
	if (pFile == NULL)
		return false;
	while (!feof(pFile)) {
		memset(buffer, 0, sizeof(buffer));
		fgets(buffer, 256, pFile);
		content += buffer;
	}
	if(pFile)
		fclose(pFile);
	if (!reader.parse(content, root)) {
		return false;
	}
	Json::Value users			= root["config"];
	for (auto & user : users) {
		NdUserInfo singleUsr;
		singleUsr.m_uname       = user["user"].asString();
		singleUsr.m_99upwd      = user["password"].asString();
		singleUsr.m_ioapwd      = user["password"].asString();
		singleUsr.m_bAutoSign   = user["bool_autosign"].asInt() == 1 ? true : false;
		singleUsr.m_bTaskClear  = user["bool_taskclear"].asInt() == 1 ? true : false;
		singleUsr.m_bBirth      = user["bool_birth"].asInt() == 1 ? true : false;
		singleUsr.m_bExtraPt    = user["bool_extraPt"].asInt() == 1 ? true : false;
		singleUsr.m_bWriteDiary = user["bool_diary"].asInt() == 1 ? true : false;
		singleUsr.m_bSendFlower = user["bool_sendflower"].asInt() == 1 ? true : false;
		if (singleUsr.m_bSendFlower) {
			for (auto item : user["recver"]) {
				NdRecverInfo snd;
				snd.m_recverId	= item["receiver"].asString();
				snd.m_count		= item["count"].asInt();
				singleUsr.m_sendFlower.emplace_back(snd);
			}
		}
		else {
			singleUsr.m_sendFlower.clear();
		}
		if (singleUsr.m_bWriteDiary) {
			auto diary_contex						= user["diary"];
			if (diary_contex == NULL) {

			}
			singleUsr.m_diaryDetail.m_Id			= diary_contex["id"].asString();
			singleUsr.m_diaryDetail.m_realName		= diary_contex["name"].asString();
			singleUsr.m_diaryDetail.m_comCode		= diary_contex["compCode"].asString();
			singleUsr.m_diaryDetail.m_depCode		= diary_contex["depCode"].asString();
			singleUsr.m_diaryDetail.m_groupCode		= diary_contex["groupCode"].asString();
			singleUsr.m_diaryDetail.m_groupName		= diary_contex["groupName"].asString();
			singleUsr.m_diaryDetail.m_content		= diary_contex["content"].asString();
			singleUsr.m_diaryDetail.m_completeRate	= diary_contex["compRate"].asString();
		}
		pUsers->emplace_back(singleUsr);
	}
	return true;
}

bool NdTask::t_RequestForUC(const std::string& userName, const std::string& pwd, NdUCToken* pToken)
{
	if (!pToken)
		return false;
	NdHttpClient http;
	http.SetRequestMethod(NdHttpClient::kPost);
	http.SetRequestUrl("https://aqapi.101.com/v0.91/tokens");
	http.SetHttpsMode(true);
	http.AddRequestHeader("Accept", "application/json");
	http.AddRequestHeader("Content-Type", "application/json");
	http.AddRequestHeader("Host", "aqapi.101.com");

	std::string tmp = "{\"login_name\":\"" + userName + "\",\"password\":\"" + pwd + "\"}";
	http.SetPostFileds(tmp);
	int code = http.ExecRequest();
	if (code != 0) {
		return false;
	}
	tmp = http.GetResponseContent();

	Json::Reader reader;
	Json::Value  value;
	if (!reader.parse(tmp, value)) {
		return false;
	}
	pToken->access_token  = value["access_token"].asString();
	pToken->expired_at    = value["expires_at"].asString();
	pToken->mac_algorithm = value["mac_algorithm"].asString();
	pToken->mac_key       = value["mac_key"].asString();
	pToken->refresh_token = value["refresh_token"].asString();
	pToken->server_time   = value["server_time"].asString();
	pToken->user_id       = value["user_id"].asString();

	return true;
}

std::string NdTask::t_CalculateAuthorthem(NdUCToken *pToken, const std::string& http_method, const std::string& host, const std::string& path)
{
	std::string nonce = NdCalculateUtil::getNowMilliSeconds() + ":" + NdCalculateUtil::generateMixRandomCode(8);

	std::ostringstream oss;
	oss << nonce << "\n" << http_method << "\n" << path << "\n" << host << "\n";
	auto rawMac = oss.str();
	std::string mac = NdCalculateUtil::encryptHMac256(rawMac, pToken->mac_key);
	oss.str("");

	oss << "MAC id=\"" << pToken->access_token << "\",nonce=\"" << nonce << "\",mac=\"" << mac << "\"";
	return oss.str();
}

bool NdTask::t_GetBlessList(NdUCToken *pToken, std::vector<NdBlessInfo> *pList, bool& retry)
{
	if (!pToken || !pList)
		return false;
	pList->clear();
	std::string auth = t_CalculateAuthorthem(pToken, "GET", "im-birthday.social.web.sdp.101.com", "/v0.1/birthday_users");
	NdHttpClient http;
	http.SetRequestMethod(NdHttpClient::kGet);
	http.SetRequestUrl("http://im-birthday.social.web.sdp.101.com/v0.1/birthday_users");
	http.AddRequestHeader("Accept", "application/json");
	http.AddRequestHeader("Authorization", auth);
	http.AddRequestHeader("Host", "im-birthday.social.web.sdp.101.com");
	http.AddRequestHeader("Tenancy", "0");
	int nCode = http.ExecRequest();
	if (nCode != 0)
		return false;
	nCode = http.GetResponseCode();

	if (nCode != 200) {
		retry = true;
		return false;
	}
	auth = http.GetResponseContent();
	Json::Reader reader;
	Json::Value  root;
	if (!reader.parse(auth, root))
	{
		retry = false;
		return false;
	}		

	Json::Value items = root["items"];
	for (auto & item : items) {
		NdBlessInfo singleInfo;
		singleInfo.node_name = item["node_name"].asString();
		singleInfo.real_name = item["real_name"].asString();
		singleInfo.user_id = item["user_id"].asString();
		pList->emplace_back(singleInfo);
	}
	retry = false;
	return true;
}

bool NdTask::t_BlessBirth(NdUCToken *pToken, const std::vector<NdBlessInfo>& BlessInfoes)
{
	if (!pToken)
		return false;
	
	std::string path = "";
	std::string auth = "";
	NdHttpClient http;
	int nCode = 0;
	Json::Reader reader;
	Json::Value value;
	bool flag = false;
	for (auto & info : BlessInfoes) {
		bool need_redo = true;
		while (need_redo) {
			path = "/v0.1/birthday_users/" + info.user_id + "/actions/bless";
			auth = t_CalculateAuthorthem(pToken, "POST", "im-birthday.social.web.sdp.101.com", path);

			http.ClearRequestState();
			http.SetRequestMethod(NdHttpClient::kPost);
			http.SetRequestUrl("http://im-birthday.social.web.sdp.101.com" + path);
			http.AddRequestHeader("Accept", "application/json");
			http.AddRequestHeader("Authorization", auth);
			http.AddRequestHeader("Host", "im-birthday.social.web.sdp.101.com");
			http.AddRequestHeader("Content-Type", "application/json");
			http.AddRequestHeader("Tenancy", "0");
			http.SetPostFileds("");
			nCode                           = http.ExecRequest();
			if (nCode != 0){
				break;
			}
				
			nCode                           = http.GetResponseCode();
			if (nCode != 200 && nCode != 409) {
				continue;
			}
			auth                            = http.GetResponseContent();
			if (!reader.parse(auth, value)) {
				return false;
			}
			if (value["code"] == "IMB/HAS_BLESSED") {
				flag                        = true;
				need_redo					= false;
				value.clear();
				break;
			}
			if (value["user_id"].asString() == info.user_id)
			{
				flag                        = true;
				need_redo					= false;
			}
		}
	}

	return flag;
}

bool NdTask::t_LoginIOA(NdUserInfo *pUserInfo)
{
	if (!pUserInfo)
		return false;
	NdHttpClient http;
	http.SetRequestMethod(NdHttpClient::kGet);
	http.SetRequestUrl("http://ioa.99.com");
	http.AddRequestHeader("Accept", "text/html, application/xhtml+xml, image/jar, */*");
	http.AddRequestHeader("Accept-Language", "en-US,en;q=0.8,zh-Hans-CN;q=0.5,zh-Hans;q=0.3");
	http.AddRequestHeader("User-Agent", "Mozilla/5.0 (Windows NT 10.0; Win64; x64) AppleWebKit/537.36 (KHTML, like Gecko) Chrome/42.0.2311.135 Safari/537.36 Edge/12.10240");
	http.AddRequestHeader("Connection", "Keep-Alive");
	http.AddRequestHeader("Host", "ioa.99.com");

	int nCode = http.ExecRequest();
	if (nCode != 0) {
		return false;
	}
	std::string content = http.GetResponseContent();
	std::string headers = http.GetResponseHeaders();
	http.ClearRequestState();


	std::string flag1   = "<input id=\"NdToken1\" name=\"NdToken1\" type=\"hidden\" value=\" ";

	//1. 处理cookie
	std::vector<std::string> tmp = NdCalculateUtil::SplitString(headers, std::string("\r\n"));
	for (unsigned int i = 0; i < tmp.size(); i++)
	{
		if (NdCalculateUtil::ContainString(tmp[i], std::string("Set-Cookie")))
		{
			std::vector<std::string> tmp2 = NdCalculateUtil::SplitString(tmp[i], std::string(":"));
			{
				std::size_t pos = tmp2[1].find("path");
				if (pos != std::string::npos)
					pUserInfo->m_ioacookie.append(tmp2[1].substr(0, pos));
				else
					pUserInfo->m_ioacookie.append(tmp2[1]);
			}
		}
	}

	std::string ndTokenPattern1 = "<input id=\"NdToken1\" name=\"NdToken1\" type=\"hidden\" value=\"";
	std::string ndTokenPattern2 = "<input id=\"NdToken2\" name=\"NdToken2\" type=\"hidden\" value=\"";
	std::string rsaPattern		= "var key = new RSAKeyPair(\"010001\", \"\", \"";
	std::string NdToken1		= "";
	std::string NdToken2		= "";
	std::string moduluses		= "";

	std::size_t ndpos = content.find(ndTokenPattern1);
	if (ndpos != std::string::npos) {
		NdToken1 = content.substr(ndpos + ndTokenPattern1.length(), 1840);
	}
	ndpos = content.find(ndTokenPattern2);
	if (ndpos != std::string::npos) {
		NdToken2 = content.substr(ndpos + ndTokenPattern2.length(), 496);
	}
	ndpos = content.find(rsaPattern);
	if (ndpos != std::string::npos) {
		moduluses = content.substr(ndpos + rsaPattern.length(), 256);
	}

	pUserInfo->m_ioacookie += "uname=" + pUserInfo->m_uname;

	// 得到cookie后，对用户名密码进性加密
	std::string base64_pwd = NdCalculateUtil::Base64Encode(pUserInfo->m_ioapwd, false);
	std::string rsa_pwd = NdCalculateUtil::RSAEncrypt(moduluses, "", base64_pwd);

	std::string viewState = "__VIEWSTATE=%2FwEPDwUKMTY4MTIyNTIxMmRk&NdToken1=" + NdToken1 + "&NdToken2=" + NdToken2 + "&Password=" + rsa_pwd + "&Password2=&rsa=1&Username=" + pUserInfo->m_uname;

	http.SetRequestMethod(NdHttpClient::kPost);
	http.SetRequestUrl("http://ioa.99.com/Default.aspx");
	http.AddRequestHeader("Accept", "text/html,application/xhtml+xml,image/jxr,*/*");
	http.AddRequestHeader("Accept-Language", "en-US,en;q=0.8,zh-Hans-CN;q=0.5,zh-Hans;q=0.3");
	http.AddRequestHeader("Cache-Control", "no-cache");
	http.AddRequestHeader("Connection", "Keep-Alive");
	http.AddRequestHeader("Content-Type", "application/x-www-form-urlencoded");
	http.AddRequestHeader("Cookie", pUserInfo->m_ioacookie);
	http.AddRequestHeader("Host", "ioa.99.com");
	http.AddRequestHeader("User-Agent", "Mozilla/5.0 (Windows NT 10.0; Win64; x64) AppleWebKit/537.36 (KHTML, like Gecko) Chrome/42.0.2311.135 Safari/537.36 Edge/12.10240");
	http.AllowAutoRedirect(false);
	http.SetPostFileds(viewState);
	nCode = http.ExecRequest();
	if (nCode != 0) {
		return false;
	}
	headers = http.GetResponseHeaders();
	ndpos = headers.find("expire");
	std::size_t ndpos2 = headers.find("user_Info=");
	if (ndpos2 != std::string::npos && ndpos != std::string::npos) {
		std::string str_cookie = headers.substr(ndpos2, ndpos - ndpos2);
		pUserInfo->m_ioacookie += ";" + str_cookie;
		return true;
	}
	return false;
}


bool NdTask::t_AutoSign(NdUserInfo *pUserInfo)
{
	if (!pUserInfo)
		return false;

	NdHttpClient http;
	http.SetRequestUrl("http://ioa.99.com/ajax/A0_swfScdaOut.aspx?action=signin&t=" + NdCalculateUtil::getNowMilliSeconds());
	http.SetRequestMethod(NdHttpClient::kGet);
	http.AddRequestHeader("Accept", "application/json, text/javascript");
	http.AddRequestHeader("Accept-Language", "en-US,en;q=0.8,zh-Hans-CN;q=0.5,zh-Hans;q=0.3");
	http.AddRequestHeader("Connection", "Keep-Alive");
	http.AddRequestHeader("Cookie", pUserInfo->m_ioacookie);
	http.AddRequestHeader("Host", "ioa.99.com");
	http.AddRequestHeader("Referer", "http://ioa.99.com/ERPDesk/Assist_Default.aspx");
	http.AddRequestHeader("User-Agent", "Mozilla/5.0 (Windows NT 10.0; Win64; x64) AppleWebKit/537.36 (KHTML, like Gecko) Chrome/42.0.2311.135 Safari/537.36 Edge/12.10240");
	http.AddRequestHeader("X-Requested-With", "XMLHttpRequest");

	int nCode = http.ExecRequest();
	if (nCode != 0) {
		return false;
	}

	std::string content = http.GetResponseContent();

	Json::Reader reader;
	Json::Value  value;

	// {"MsgKey":1,"MsgContent":"成功签到：获得<font color='red'>200</font>经验，<font color='red'>200</font>积分，<font color='red'>5</font>朵红玫瑰"}
	if (!reader.parse(content, value)) {
		std::printf("[%s][%d][%s]\n", __FUNCTION__, http.GetResponseCode(), content.c_str());
		return false;
	}
	int tmp = value["MsgKey"].asInt();
	if (tmp == 1 || tmp == -1) {
		
		return true;
	}
	std::printf("[%s][%d][%s]\n", __FUNCTION__, http.GetResponseCode(), content.c_str());
	return false;
}

bool NdTask::t_DayTaskClear(NdUserInfo *pUserInfo)
{
	if (!pUserInfo)
		return false;

	NdHttpClient http;
	http.SetRequestUrl("http://ioa.99.com/ajax/A0_swfScdaOut.aspx?action=signout&t=" + NdCalculateUtil::getNowMilliSeconds());
	http.SetRequestMethod(NdHttpClient::kGet);
	http.AddRequestHeader("Accept", "application/json, text/javascript");
	http.AddRequestHeader("Accept-Language", "en-US,en;q=0.8,zh-Hans-CN;q=0.5,zh-Hans;q=0.3");
	http.AddRequestHeader("Connection", "Keep-Alive");
	http.AddRequestHeader("Cookie", pUserInfo->m_ioacookie);
	http.AddRequestHeader("Host", "ioa.99.com");
	http.AddRequestHeader("Referer", "http://ioa.99.com/ERPDesk/Assist_Default.aspx");
	http.AddRequestHeader("User-Agent", "Mozilla/5.0 (Windows NT 10.0; Win64; x64) AppleWebKit/537.36 (KHTML, like Gecko) Chrome/42.0.2311.135 Safari/537.36 Edge/12.10240");
	http.AddRequestHeader("X-Requested-With", "XMLHttpRequest");

	int nCode = http.ExecRequest();
	if (nCode != 0) {
		return false;
	}

	std::string content = http.GetResponseContent();

	Json::Reader reader;
	Json::Value  value;

	// {"MsgKey":"1" 还没有签到呢} {"MsgKey":"0" 已经签到过了}
	if (!reader.parse(content, value)) {
		std::printf("[%s][%d][%s]\n", __FUNCTION__, http.GetResponseCode(), content.c_str());
		return false;
	}
	auto msgKey = value["MsgKey"].asString();
	if (msgKey == "1" || msgKey == "0")
		return true;
	NdCodeConvert  Convert("gb2312", "utf-8");
	std::string cont = value["MsgContent"].asString();
	char ccont[128] = {0};
	int ccont_length = 0;
	Convert.convert((char *)cont.c_str(), cont.size(), ccont, sizeof(ccont));
	std::printf("[%s][%s][%d][%s]\n", pUserInfo->m_uname.c_str(), __FUNCTION__, http.GetResponseCode(), ccont);
	return false;
}

bool NdTask::t_SendFlower(NdUCToken *pToken, const std::string& recvId, int count, bool& retry)
{
	if (!pToken)
		return false;
	std::string auth = t_CalculateAuthorthem(pToken, "POST", "pack.web.sdp.101.com", "/c/flower/send");
	if (auth == "")
		return false;
	retry = false;
	NdHttpClient http;
	http.SetRequestMethod(NdHttpClient::kPost);
	http.SetRequestUrl("http://pack.web.sdp.101.com/c/flower/send");
	http.AddRequestHeader("Accept", "application/json");
	http.AddRequestHeader("Authorization", auth);
	http.AddRequestHeader("Content-Type", "application/json");
	http.AddRequestHeader("Host", "pack.web.sdp.101.com");

	std::string tmp = "{\"amount\": " + std::to_string(count) + ",\"dest_uid\":\"" + recvId + "\", \"item_type_id\":20000}";
	http.SetPostFileds(tmp);
	int nCode = http.ExecRequest();
	if (nCode != 0){
		retry = false;
		return false;
	}
	nCode = http.GetResponseCode();
	if (nCode != 200){
		retry = true;
		//std::cout <<" send flower log : response code = " << nCode << "| content = " << http.GetResponseContent() << std::endl;
		return false;
	}
	tmp = http.GetResponseContent();
	Json::Reader reader;
	Json::Value value;
	if (!reader.parse(tmp, value))
	{
		std::printf("[%s][%d][%s]\n", __FUNCTION__, http.GetResponseCode(), tmp.c_str());
		return false;
	}
		
	if (value["type"].asInt() == 0 || value["type"].asInt() == 1)
		return true;
	std::printf("[%s][%d][%s]\n", __FUNCTION__, http.GetResponseCode(), tmp.c_str());
	return false;
}



bool NdTask::t_GetExtraList(NdUserInfo *pUserInfo, std::vector<NdExtraPointInfo> *pExtraInfoes)
{
	if (!pUserInfo || !pExtraInfoes)
		return false;
	
	time_t t				= time(NULL);
	char timeInterval[32] = {0};
	std::sprintf(timeInterval, "%ld356", t + 24 * 60 * 60 * 1000);
	std::string tom(timeInterval);

	NdHttpClient http;
	//http.SetRequestUrl("http://ioa.99.com/ajax/A0_swfScdaOut.aspx?action=GetReceiveList&t=" + NdCalculateUtil::getNowMilliSeconds());
	http.SetRequestUrl("http://ioa.99.com/ajax/A0_swfScdaOut.aspx?action=GetReceiveList&t=" + tom);
	http.SetRequestMethod(NdHttpClient::kGet);
	http.AddRequestHeader("Accept", "application/json, text/javascript");
	http.AddRequestHeader("Accept-Language", "en-US,en;q=0.8,zh-Hans-CN;q=0.5,zh-Hans;q=0.3");
	http.AddRequestHeader("Connection", "Keep-Alive");
	http.AddRequestHeader("Cookie", pUserInfo->m_ioacookie);
	http.AddRequestHeader("Host", "ioa.99.com");
	http.AddRequestHeader("Referer", "http://ioa.99.com/ERPDesk/Assist_Default.aspx?uid=0");
	http.AddRequestHeader("User-Agent", "Mozilla/5.0 (Windows NT 10.0; Win64; x64) AppleWebKit/537.36 (KHTML, like Gecko) Chrome/42.0.2311.135 Safari/537.36 Edge/12.10240");
	http.AddRequestHeader("X-Requested-With", "XMLHttpRequest");

	int nCode = http.ExecRequest();
	if (nCode != 0) {
		return false;
	}

	std::string content = http.GetResponseContent();

	Json::Reader reader;
	Json::Value  value;

	// {"MsgKey":"1"}
	if (!reader.parse(content, value)) {
		std::printf("[%s][%d][%s]\n", __FUNCTION__, http.GetResponseCode(), content.c_str());
		return false;
	}

	if (value["MsgKey"].asInt() != 1) {
		std::printf("[%s][%d][%s]\n", __FUNCTION__, http.GetResponseCode(), content.c_str());
		return false;
	}

	Json::Value items = value["Items"];
	for (auto item : items) {
		if (item["State"].asString() == "0")
		{
			NdExtraPointInfo ptInfo;
			ptInfo.m_code  = item["Code"].asString();
			ptInfo.m_name  = item["Name"].asString();
			ptInfo.m_point = item["Point"].asString();
			ptInfo.m_exp   = item["Exp"].asString();
			ptInfo.m_state = item["State"].asString();
		
			pExtraInfoes->emplace_back(ptInfo);
		}		
	}

	return true;
}

bool NdTask::t_GetExtraPoint(NdUserInfo *pUserInfo, const std::vector<NdExtraPointInfo>& infos)
{
	if (!pUserInfo)
		return false;
	NdHttpClient http;
	std::string tmp = "";

	std::printf("[%s] extra points [%d]\n", pUserInfo->m_uname.c_str(), infos.size());
	for (auto && item : infos)
	{
		http.ClearRequestState();
		http.SetRequestUrl("http://ioa.99.com/ajax/A0_swfScdaOut.aspx?action=getpoint&t=" + NdCalculateUtil::getNowMilliSeconds());
		http.SetRequestMethod(NdHttpClient::kPost);
		http.AddRequestHeader("Accept", "application/json, text/javascript, */*; q=0.01");
		http.AddRequestHeader("Accept-Language", "zh-CN,zh;q=0.8,en-US;q=0.5,en;q=0.3");
		http.AddRequestHeader("Cache-Control", "no-cache");
		http.AddRequestHeader("Connection", "Keep-Alive");
		http.AddRequestHeader("Cookie", pUserInfo->m_ioacookie);
		http.AddRequestHeader("Content-Type", "application/x-www-form-urlencoded");
		http.AddRequestHeader("Host", "ioa.99.com");
		http.AddRequestHeader("Referer", "http://ioa.99.com/ERPDesk/Assist_Default.aspx");
		http.AddRequestHeader("User-Agent", "Mozilla/5.0 (Windows NT 10.0; Win64; x64) AppleWebKit/537.36 (KHTML, like Gecko) Chrome/42.0.2311.135 Safari/537.36 Edge/12.10240");
		http.AddRequestHeader("Pragma", "no-cache");
		http.AddRequestHeader("X-Requested-With", "XMLHttpRequest");
		tmp = "code=" + item.m_code;
		http.SetPostFileds(tmp);
		int nCode = http.ExecRequest();
		if (nCode != 0) {
			return false;
		}
		nCode = http.GetResponseCode();
		if (nCode != 200)
		{
			std::printf("[%s][%d][%s]\n", __FUNCTION__, http.GetResponseCode(), tmp.c_str());
			continue;
		}
		tmp = http.GetResponseContent();
		Json::Reader reader;
		Json::Value  value;
		if (!reader.parse(tmp, value)) {
			std::printf("[%s][%d][%s]\n", __FUNCTION__, http.GetResponseCode(), tmp.c_str());
			continue;
		}
		if (value["MsgKey"].asInt() == 1) {
			std::printf("[%s][%d][%s]\n", __FUNCTION__, http.GetResponseCode(), tmp.c_str());
			continue;
		}	
		NdCodeConvert  Convert("gb2312", "utf-8");
		std::string cont = value["Content"].asString();
		char ccont[128] = {0};
		int ccont_length = 0;
		Convert.convert((char *)cont.c_str(), cont.size(), ccont, sizeof(ccont));
		std::printf("[%s][%d][%s]\n", __FUNCTION__, http.GetResponseCode(), ccont);
	}

	return true;
}


bool NdTask::t_WriteDiray(NdUserInfo* pUserInfo)
{
	NdDirayContent content = std::move(pUserInfo->m_diaryDetail);
	NdHttpClient http;
	http.SetRequestUrl("http://ioa.99.com/Ajax/A0_frmScdaOut.aspx");
	http.SetRequestMethod(NdHttpClient::kPost);
	http.AddRequestHeader("Accept", "*/*");
	http.AddRequestHeader("If-Modified-Since", "0");
	http.AddRequestHeader("Content-Type", "application/x-www-form-urlencoded");
	http.AddRequestHeader("Referer", "http://ioa.99.com/Rcweb/K7_frmRcPlan.aspx");
	http.AddRequestHeader("Accept-Language", "en-US,en;q=0.8,zh-Hans-CN;q=0.5,zh-Hans;q=0.3");
	http.AddRequestHeader("User-Agent", "Mozilla/5.0 (Windows NT 10.0; WOW64; Trident/7.0; rv:11.0) like Gecko");
	http.AddRequestHeader("Host", "ioa.99.com");
	http.AddRequestHeader("Connection", "Keep-Alive");
	http.AddRequestHeader("Cache-Control", "no-cache");
	http.AddRequestHeader("Cookie", pUserInfo->m_ioacookie);

	std::string today = NdCalculateUtil::getCurrentSystemTime();

	//std::string content = "frmName=K7_frmRcPlan&action=save&txtArgs=9153B0DB2A6B8271∮张新洲∮01023I0N02∮工程院技术开发部前端开发处∮2015-10-27∮1∮0∮0∮0∮∮∮Y∮1&mainData=∮2015-10-27∮正常单据∮121416∮张新洲∮∮∮8&logData=§OR§OR001§01023I0N02§wifi下组播测试§8§35§50§§§§§1§&ajaxCache=1445932374145";
	std::string tmp = "frmName=K7_frmRcPlan&action=save&txtArgs="+ content.m_Id +"∮"+ content.m_realName + "∮" + content.m_groupCode +"∮" + content.m_groupName + "∮"+ today +"∮1∮0∮0∮0∮∮∮Y∮1&mainData=∮"+today+"∮正常单据∮"+pUserInfo->m_uname+"∮" + content.m_realName +"∮∮∮8&logData=§"+content.m_comCode+"§"+content.m_depCode+"§"+content.m_groupCode+"§"+content.m_content+"§8§"+ content.m_completeRate+"§50§§§§§1§&ajaxCache=" + NdCalculateUtil::getNowMilliSeconds();
	http.SetPostFileds(tmp);

	int nCode = http.ExecRequest();
	if (nCode != 0) {
		return false;
	}
	nCode = http.GetResponseCode();
	if (nCode != 200) {
		return false;
	}
	return true;
}




