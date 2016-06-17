#ifndef NDTASK_H
#define NDTASK_H

#include <string>
#include <vector>


typedef struct _tag_UCToken {
	std::string		access_token;
	std::string		expired_at;
	std::string		mac_algorithm;
	std::string		mac_key;
	std::string		refresh_token;
	std::string		server_time;
	std::string		user_id;
}NdUCToken;

typedef struct _tag_BlessInfo {
	int				node_id;			// 部门ID号
	std::string		node_name;			// 部门名称
	std::string		real_name;			// 真实姓名
	std::string		user_id;			// 用户ID
}NdBlessInfo;

typedef struct _tag_RecverInfo {
	std::string		m_recverId;
	int				m_count;
}NdRecverInfo;

typedef struct _tag_dirayContent
{
	std::string		m_Id;				// ERP对应的ID
	std::string		m_comCode;			// 大部分编号，例如工程院 ，游戏部等
	std::string		m_depCode;			// 部门标号,例如工程院IM
	std::string		m_groupCode;		// 小组编号，例如工程院IM服务端
	std::string		m_groupName;		// 小组名称，例如工程院技术开发部前端开发处
	std::string		m_realName;			// 真实姓名
	std::string		m_content;			// 日志内容
	std::string		m_completeRate;		// 完成百分率
}NdDirayContent;


typedef struct _tag_UserInfo
{
	std::string					m_uname;			// 新99u用户名
	std::string					m_ioapwd;			// ioa用户密码
	std::string					m_99upwd;			// 99u用户密码
	std::string					m_ioasid;			// ioasid访问密码
	std::string					m_ioacookie;		// ioacookie
	std::vector<NdRecverInfo>	m_sendFlower;		// 送花相关信息
	std::string					m_lastErrMsg;		// 处理错误内容
	bool						m_bExtraPt;			// 是否执行额外分数
	bool						m_bTaskClear;		// 是否执行日事日清
	bool						m_bAutoSign;		// 是否执行自动签到
	bool						m_bSendFlower;		// 是否执行送花
	bool						m_bBirth;			// 是否执行生日祝福
	bool						m_bWriteDiary;		// 是否自动写日志
	NdDirayContent				m_diaryDetail;		// 日志所需信息
}NdUserInfo;

typedef struct _tag_extraPointInfo {
	std::string		m_code;				// 工号
	std::string		m_name;				// 员工名
	std::string		m_point;			// 积分
	std::string		m_exp;				// 经验
	std::string		m_state;			// 状态  m_state==0 未完成 m_state==1已完成
}NdExtraPointInfo;



class NdTask
{
public:
	NdTask()	= default;
	~NdTask()	= default;
	
	bool operator()(NdUserInfo& user);
	
	
	static bool InitUsers(const char* config_path, std::vector<NdUserInfo> *pUsers);
private:
	
	/***
	*			向UC中心请求访问Token
	* @userName	： 请求用户名
	* @pwd		:  请求用户密码（MD5加密过的）
	* @pToken	:  用户Token内容（输出）
	* @return	:  正常获取Token，返回true；失败返回false
	*/
	bool t_RequestForUC(const std::string& userName, const std::string& pwd, NdUCToken* pToken);
	
	/*** 
	*			计算UC请求携带的MAC
	* @NdUCToken	: 用户请求Token（输入）
	* @std::string	: 用户http请求方法（只能为大写，GET，POST,HEAD等等）
	* @host			: 用户请求的主机名
	* @path			: 用户请求的剩余路径内容
	* @return		: 成功返回MAC内容，失败返回空串
	*/
	std::string t_CalculateAuthorthem(NdUCToken *pToken, const std::string& http_method, const std::string& host, const std::string& path);
	
	/***
	*			获取生日祝福列表
	* @pToken		: 用户请求Token（输入）
	* @pList		: 生日祝福信息表（输出)
	* @return		: 成功返回true，失败返回false
	*/
	bool t_GetBlessList(NdUCToken *pToken, std::vector<NdBlessInfo> *pList, bool& retry);
	
	/***
	*			生日祝福
	* @pToken		: 用户请求Token（输入）
	* @pList		: 生日注入列表信息（出入）
	* @return		: 成功返回true，失败返回false
	*/
	bool t_BlessBirth(NdUCToken *pToken, const std::vector<NdBlessInfo>& BlessInfoes);
	
	/***
	*			登陆IOA
	* @pUserInfo	: 登陆IOA需要的用户信息
	* @return		: 成功true，失败false
	*/
	bool t_LoginIOA(NdUserInfo *pUserInfo);
	
	/***
	*			自动签到
	* @pUserInfo	: IOA用户信息
	* @return		: 成功true,失败返回false
	*/
	bool t_AutoSign(NdUserInfo *pUserInfo);
	
	
	/***
	*			日事日清理
	* @pUserInfo	: IOA用户信息
	* @return		: 成功true,失败返回false
	*/
	bool t_DayTaskClear(NdUserInfo *pUserInfo);

	/***
	*			送花
	* @pToken		: 用户请求Token（输入）
	* @recvId		: 送花给谁
	* @count		: 送花数量
	* @return		: 成功送花true，失败返回false
	*/
	bool t_SendFlower(NdUCToken *pToken, const std::string& recvId, int count, bool& retry);
	
	/***
	*			得到额外分数
	* @pUserInfo	: 登陆IOA需要的用户信息
	* @pExtraInfoes	: 额外分数列表（输出）
	* @return		: 成功获取true，否则返回false
	*/
	bool t_GetExtraList(NdUserInfo *pUserInfo, std::vector<NdExtraPointInfo> *pExtraInfoes);
	
	/***
	*			获取额外分数
	* @pUserInfo	: 登陆IOA需要的用户信息
	* @pExtraInfoes	: 额外分数列表
	* @return		: 成功获取true，否则返回false
	*/
	bool t_GetExtraPoint(NdUserInfo *pUserInfo, const std::vector<NdExtraPointInfo>& infos);
	
	/****
	*			自动书写日志
	* @pUserInfo	: 登陆IOA需要的用户信息
	* @content		： 书写日志的具体内容
	* @return		: 成功返回true，否则返回false
	*/
	bool t_WriteDiray(NdUserInfo* pUserInfo);
	
};


#endif // NDTASK_H
