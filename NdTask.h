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
	int				node_id;			// ����ID��
	std::string		node_name;			// ��������
	std::string		real_name;			// ��ʵ����
	std::string		user_id;			// �û�ID
}NdBlessInfo;

typedef struct _tag_RecverInfo {
	std::string		m_recverId;
	int				m_count;
}NdRecverInfo;

typedef struct _tag_dirayContent
{
	std::string		m_Id;				// ERP��Ӧ��ID
	std::string		m_comCode;			// �󲿷ֱ�ţ����繤��Ժ ����Ϸ����
	std::string		m_depCode;			// ���ű��,���繤��ԺIM
	std::string		m_groupCode;		// С���ţ����繤��ԺIM�����
	std::string		m_groupName;		// С�����ƣ����繤��Ժ����������ǰ�˿�����
	std::string		m_realName;			// ��ʵ����
	std::string		m_content;			// ��־����
	std::string		m_completeRate;		// ��ɰٷ���
}NdDirayContent;


typedef struct _tag_UserInfo
{
	std::string					m_uname;			// ��99u�û���
	std::string					m_ioapwd;			// ioa�û�����
	std::string					m_99upwd;			// 99u�û�����
	std::string					m_ioasid;			// ioasid��������
	std::string					m_ioacookie;		// ioacookie
	std::vector<NdRecverInfo>	m_sendFlower;		// �ͻ������Ϣ
	std::string					m_lastErrMsg;		// �����������
	bool						m_bExtraPt;			// �Ƿ�ִ�ж������
	bool						m_bTaskClear;		// �Ƿ�ִ����������
	bool						m_bAutoSign;		// �Ƿ�ִ���Զ�ǩ��
	bool						m_bSendFlower;		// �Ƿ�ִ���ͻ�
	bool						m_bBirth;			// �Ƿ�ִ������ף��
	bool						m_bWriteDiary;		// �Ƿ��Զ�д��־
	NdDirayContent				m_diaryDetail;		// ��־������Ϣ
}NdUserInfo;

typedef struct _tag_extraPointInfo {
	std::string		m_code;				// ����
	std::string		m_name;				// Ա����
	std::string		m_point;			// ����
	std::string		m_exp;				// ����
	std::string		m_state;			// ״̬  m_state==0 δ��� m_state==1�����
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
	*			��UC�����������Token
	* @userName	�� �����û���
	* @pwd		:  �����û����루MD5���ܹ��ģ�
	* @pToken	:  �û�Token���ݣ������
	* @return	:  ������ȡToken������true��ʧ�ܷ���false
	*/
	bool t_RequestForUC(const std::string& userName, const std::string& pwd, NdUCToken* pToken);
	
	/*** 
	*			����UC����Я����MAC
	* @NdUCToken	: �û�����Token�����룩
	* @std::string	: �û�http���󷽷���ֻ��Ϊ��д��GET��POST,HEAD�ȵȣ�
	* @host			: �û������������
	* @path			: �û������ʣ��·������
	* @return		: �ɹ�����MAC���ݣ�ʧ�ܷ��ؿմ�
	*/
	std::string t_CalculateAuthorthem(NdUCToken *pToken, const std::string& http_method, const std::string& host, const std::string& path);
	
	/***
	*			��ȡ����ף���б�
	* @pToken		: �û�����Token�����룩
	* @pList		: ����ף����Ϣ�����)
	* @return		: �ɹ�����true��ʧ�ܷ���false
	*/
	bool t_GetBlessList(NdUCToken *pToken, std::vector<NdBlessInfo> *pList, bool& retry);
	
	/***
	*			����ף��
	* @pToken		: �û�����Token�����룩
	* @pList		: ����ע���б���Ϣ�����룩
	* @return		: �ɹ�����true��ʧ�ܷ���false
	*/
	bool t_BlessBirth(NdUCToken *pToken, const std::vector<NdBlessInfo>& BlessInfoes);
	
	/***
	*			��½IOA
	* @pUserInfo	: ��½IOA��Ҫ���û���Ϣ
	* @return		: �ɹ�true��ʧ��false
	*/
	bool t_LoginIOA(NdUserInfo *pUserInfo);
	
	/***
	*			�Զ�ǩ��
	* @pUserInfo	: IOA�û���Ϣ
	* @return		: �ɹ�true,ʧ�ܷ���false
	*/
	bool t_AutoSign(NdUserInfo *pUserInfo);
	
	
	/***
	*			����������
	* @pUserInfo	: IOA�û���Ϣ
	* @return		: �ɹ�true,ʧ�ܷ���false
	*/
	bool t_DayTaskClear(NdUserInfo *pUserInfo);

	/***
	*			�ͻ�
	* @pToken		: �û�����Token�����룩
	* @recvId		: �ͻ���˭
	* @count		: �ͻ�����
	* @return		: �ɹ��ͻ�true��ʧ�ܷ���false
	*/
	bool t_SendFlower(NdUCToken *pToken, const std::string& recvId, int count, bool& retry);
	
	/***
	*			�õ��������
	* @pUserInfo	: ��½IOA��Ҫ���û���Ϣ
	* @pExtraInfoes	: ��������б������
	* @return		: �ɹ���ȡtrue�����򷵻�false
	*/
	bool t_GetExtraList(NdUserInfo *pUserInfo, std::vector<NdExtraPointInfo> *pExtraInfoes);
	
	/***
	*			��ȡ�������
	* @pUserInfo	: ��½IOA��Ҫ���û���Ϣ
	* @pExtraInfoes	: ��������б�
	* @return		: �ɹ���ȡtrue�����򷵻�false
	*/
	bool t_GetExtraPoint(NdUserInfo *pUserInfo, const std::vector<NdExtraPointInfo>& infos);
	
	/****
	*			�Զ���д��־
	* @pUserInfo	: ��½IOA��Ҫ���û���Ϣ
	* @content		�� ��д��־�ľ�������
	* @return		: �ɹ�����true�����򷵻�false
	*/
	bool t_WriteDiray(NdUserInfo* pUserInfo);
	
};


#endif // NDTASK_H
