
#ifndef NDHTTPCLIENT2_H
#define NDHTTPCLIENT2_H

#include <string>
#include <map>
#include <curl/curl.h>

class NdHttpClient
{
public:
    enum eRequestMethod
    {
        kHead       = 0,            // HEAD方式请求
        kGet        = 1,            // GET方式请求
        kPost       = 2,            // POST方式请求
        kPostForm   = 3             // Post表单
    };
    NdHttpClient();
    virtual ~NdHttpClient();

    static void GlobalInit();			
	static void GlobalUnInit();

    /**
    * 设置请求路径
    */
    void SetRequestUrl(const std::string& url);

    /**
    * 设置请求方法
    * @method   : 请求的方法
    */
    void SetRequestMethod(eRequestMethod method);

    /**
    * 增加请求头
    * @key   : 键值
    * @value : 值名
    */
    void AddRequestHeader(const std::string& key, const std::string& value);

    /**
    * 遇到302 是否跟随自动跳转
    * @bFollow : 是否跟随跳转 true:跟随, false 不跳转
    */
    void AllowAutoRedirect(bool bFollow);

    /**
    *  增加请求表单
    *@form      : 表单名字
    *@content   : 表单内容

    void AddRequestFormData(const std::string& form, const std::string& content);
    */

    /**
    * 获取回应内容
    */
    std::string GetResponseContent() const;

    /**
    * 设置链接超时时间
    */
    void  SetConnectTimeout(unsigned int timeOut);

    /**
    *　设置回应超时时间
    */
    void  SetTimeout(unsigned int timeOut);
	
	/**
	* 准备HTTPS连接通讯
	*/
	void SetHttpsMode(bool isHttps);


    /**
    * 获取回应头
    */
    std::string GetResponseHeaders() const;

    /**
    * 通过Header的头来得到值
    */
    std::string  GetResponseHeaderByKey(const std::string& key);
	
	

    /**
     * 获取请求后的回应码
     */
    int GetResponseCode();

    /**
    * 清理所有请求的状态内容
    */
    void ClearRequestState();

    /**
    * 执行请求
    * @ret : 错误代码
    */
    int ExecRequest();

    /**
    * 设置Post方式发送的数据域
    */
    void SetPostFileds(const std::string & postContent);

    /**
    * 是否进入调试模式
    * @bDebug   : 是否进入调试模式
    */
    void SetDebug(bool bDebug)
    {
        m_bDebug = bDebug;
    }
private:
    static size_t OnDebug(CURL*, curl_infotype itype, char* pData, size_t size, void*);
    static size_t OnWriteData(void* buffer, size_t size, size_t nmemb, void* lpVoid);
    static size_t OnWriteHeaderData(void *buffer, size_t size, size_t nmemb, void *lpVoid);

    void AddToHeaderMap(const std::string& headerStr);
    NdHttpClient(const NdHttpClient& rhs)
    {

    }
private:
    typedef std::map<std::string, std::string>  	HttpHeaders;
    typedef HttpHeaders::iterator               	HttpHeaderIt;

    CURL                        *m_curlContext;         // curl上下文
    HttpHeaders         		m_requestHeaders;       // Http请求使用的头部
    HttpHeaders                 m_responseHeadersMap;   // Http请求获得服务器的头部
    std::string         		m_url;                  // Http请求的Url地址
    bool                		m_followLocation;       // 是否跟随跳转

    eRequestMethod      	    m_method;               // 请求的方法
    std::string         		m_responseContent;      // 回应内容
    std::string         		m_responseHeaders;      // 回应头

    unsigned int        		m_connectTimeout;      // 链接超时时间
    unsigned int        		m_timeout;

    bool                		m_bDebug;               // 是否调试模式
	bool						m_isHttps;

    std::string         		m_postFields;           // Post的数据域

    struct curl_slist    		*m_headers;             // 请求头列表


};

#endif // NDHTTPCLIENT2_H