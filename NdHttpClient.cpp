//
// Created by joole on 15-6-30.
//

#include "NdHttpClient.h"

NdHttpClient::NdHttpClient()
        : m_curlContext(0)              // 默认为空
        , m_url("")                     // 请求地址
        , m_followLocation(true)        // 跟随
        , m_method(kGet)                // 请求方法
        , m_responseContent("")         // 响应内容
        , m_connectTimeout(60)          // 默认60s连接超时
        , m_timeout(60)                 // 默认60s超时
        , m_bDebug(false)               // 是否处于调试状态
        , m_postFields("")              // post的数据区域
        , m_headers(0)                  // curl中请求头
		, m_isHttps(false)
{
    m_requestHeaders.clear();
}

NdHttpClient::~NdHttpClient()
{
	if (m_headers)
		curl_slist_free_all(m_headers);
	m_headers			= 0;
    m_requestHeaders.clear();
    m_responseHeadersMap.clear();
    if(m_curlContext)
        curl_easy_cleanup(m_curlContext);
    m_curlContext       = 0;
}

void NdHttpClient::GlobalInit()
{
    curl_global_init(CURL_GLOBAL_ALL);
}

void NdHttpClient::GlobalUnInit()
{
	curl_global_cleanup();
}

void NdHttpClient::SetRequestUrl(const std::string& url)
{
    m_url       = url;
}
void NdHttpClient::SetRequestMethod(eRequestMethod method)
{
    m_method    = method;
}

void NdHttpClient::SetHttpsMode(bool isHttps)
{
	m_isHttps = isHttps;
}

void NdHttpClient::AddRequestHeader(const std::string& key, const std::string& value)
{
    HttpHeaderIt it = m_requestHeaders.find(key);
    if(it != m_requestHeaders.end())
    {
        it->second   = value;
        return;
    }
    m_requestHeaders.insert(std::make_pair(key, value));
}

void NdHttpClient::AllowAutoRedirect(bool bFollow)
{
    m_followLocation = bFollow;
}

void NdHttpClient::SetConnectTimeout(unsigned int timeOut)
{
    m_connectTimeout = timeOut;
}

void NdHttpClient::SetTimeout(unsigned int timeOut)
{
    m_timeout        = timeOut;
}

void NdHttpClient::SetPostFileds(const std::string& postContent)
{
    m_postFields = postContent;
}

/*
void NdHttpClient::AddRequestFormData(const std::string& form, const std::string& content)
{
    PostFormsIt it = m_postForms.find(form);
    if(it != m_postForms.end())
    {
        it->second = content;
    }
    else
        m_postForms.insert(std::make_pair(form, content));
}
*/

#define CHECK_CURL_RET(x) if(x != CURLE_OK){  retCode=x;break;}

int NdHttpClient::ExecRequest()
{
    CURLcode retCode;
    //struct curl_httppost *formpost = 0;
    //struct curl_httppost *lastptr  = 0;
    // curl上下文初始化
    m_curlContext = curl_easy_init();
    do
    {
        if(!m_curlContext)
            return CURLE_FAILED_INIT;

        if(m_url.empty())
        {
            return CURLE_FAILED_INIT;
        }
        CHECK_CURL_RET(curl_easy_setopt(m_curlContext, CURLOPT_URL, m_url.c_str()));
        //设置调试模式
        if(m_bDebug)
        {
            CHECK_CURL_RET(curl_easy_setopt(m_curlContext, CURLOPT_VERBOSE, 1));
            CHECK_CURL_RET(curl_easy_setopt(m_curlContext, CURLOPT_DEBUGFUNCTION, OnDebug));
        }
	    
	    if (m_isHttps)
	    {
		    CHECK_CURL_RET(curl_easy_setopt(m_curlContext, CURLOPT_SSL_VERIFYPEER, 0L));
		    CHECK_CURL_RET(curl_easy_setopt(m_curlContext, CURLOPT_SSL_VERIFYHOST, 0L));
	    }

        //设置请求方法
        switch(m_method)
        {
            /*
            case kPostForm:
            {
                for(PostFormsIt it = m_postForms.begin(); it != m_postForms.end(); it++)
                {
                    curl_formadd(&formpost, &lastptr, CURLFORM_PTRNAME, it->first.c_str(), CURLFORM_PTRCONTENTS, it->second.c_str(), CURLFORM_END);
                }
                CHECK_CURL_RET(curl_easy_setopt(m_curlContext, CURLOPT_HTTPPOST, formpost));
                // 设置获取应答头
                CHECK_CURL_RET(curl_easy_setopt(m_curlContext, CURLOPT_HEADERFUNCTION, OnWriteHeaderData));
                CHECK_CURL_RET(curl_easy_setopt(m_curlContext, CURLOPT_HEADERDATA, (void *)&m_responseHeaders));
                // 设置回应回调函数及数据指针
                CHECK_CURL_RET(curl_easy_setopt(m_curlContext, CURLOPT_WRITEFUNCTION, OnWriteData));
                CHECK_CURL_RET(curl_easy_setopt(m_curlContext, CURLOPT_WRITEDATA, &m_responseContent));
                // 本地读取无动作
                CHECK_CURL_RET(curl_easy_setopt(m_curlContext, CURLOPT_READFUNCTION, 0));
                CHECK_CURL_RET(curl_easy_setopt(m_curlContext, CURLOPT_READDATA, 0));
            }
            break;
            */
            case kPost:
            {
                // 设置Post数据
                CHECK_CURL_RET(curl_easy_setopt(m_curlContext, CURLOPT_POST, 1));
                CHECK_CURL_RET(curl_easy_setopt(m_curlContext, CURLOPT_POSTFIELDS, m_postFields.c_str()));
                CHECK_CURL_RET(curl_easy_setopt(m_curlContext, CURLOPT_POSTFIELDSIZE, m_postFields.length()));

                // 设置获取应答头
                CHECK_CURL_RET(curl_easy_setopt(m_curlContext, CURLOPT_HEADERFUNCTION, OnWriteHeaderData));
                //CHECK_CURL_RET(curl_easy_setopt(m_curlContext, CURLOPT_HEADERDATA, (void *)&m_responseHeaders));
                CHECK_CURL_RET(curl_easy_setopt(m_curlContext, CURLOPT_HEADERDATA, (void *)this));
                // 设置回应回调函数及数据指针
                CHECK_CURL_RET(curl_easy_setopt(m_curlContext, CURLOPT_WRITEFUNCTION, OnWriteData));
                CHECK_CURL_RET(curl_easy_setopt(m_curlContext, CURLOPT_WRITEDATA, &m_responseContent));
                // 本地读取无动作
                CHECK_CURL_RET(curl_easy_setopt(m_curlContext, CURLOPT_READFUNCTION, 0));
                CHECK_CURL_RET(curl_easy_setopt(m_curlContext, CURLOPT_READDATA, 0));
            }
                break;
            case kGet:
            {
                // 设置获取应答头
                CHECK_CURL_RET(curl_easy_setopt(m_curlContext, CURLOPT_HEADERFUNCTION, OnWriteHeaderData));
                //CHECK_CURL_RET(curl_easy_setopt(m_curlContext, CURLOPT_HEADERDATA, (void *)&m_responseHeaders));
                CHECK_CURL_RET(curl_easy_setopt(m_curlContext, CURLOPT_HEADERDATA, (void *)this));
                // 设置回应回调函数及数据指针
                CHECK_CURL_RET(curl_easy_setopt(m_curlContext, CURLOPT_WRITEFUNCTION, OnWriteData));
                CHECK_CURL_RET(curl_easy_setopt(m_curlContext, CURLOPT_WRITEDATA, &m_responseContent));
                // 本地读取无动作
                CHECK_CURL_RET(curl_easy_setopt(m_curlContext, CURLOPT_READFUNCTION, 0));
            }
                break;
            case kHead:
            {
                // 设置获取应答头
                CHECK_CURL_RET(curl_easy_setopt(m_curlContext, CURLOPT_HEADERFUNCTION, OnWriteHeaderData));
                CHECK_CURL_RET(curl_easy_setopt(m_curlContext, CURLOPT_HEADERDATA, (void *)this));
                // 不收取BODY内容
                CHECK_CURL_RET(curl_easy_setopt(m_curlContext, CURLOPT_NOBODY, true));
            }
                break;
            default:
                break;
        }

        //设置请求头
        std::string  tmp;
        for(HttpHeaderIt it = m_requestHeaders.begin(); it != m_requestHeaders.end(); ++it)
        {
            tmp         = it->first + ":" + it->second;
            m_headers   = curl_slist_append(m_headers, tmp.c_str());
        }
        CHECK_CURL_RET(curl_easy_setopt(m_curlContext, CURLOPT_HTTPHEADER, m_headers));

        /**
        * 当多个现成使用超时处理的时候,同时主线程中有sleep 或是 wait等操作
        * 如果不设置这个选项,libcurl将会发信号打断这个wait从而导致程序退出
        */
        CHECK_CURL_RET(curl_easy_setopt(m_curlContext, CURLOPT_NOSIGNAL, 1));
        CHECK_CURL_RET(curl_easy_setopt(m_curlContext, CURLOPT_CONNECTTIMEOUT, m_connectTimeout));
        CHECK_CURL_RET(curl_easy_setopt(m_curlContext, CURLOPT_TIMEOUT, m_timeout));
        CHECK_CURL_RET(curl_easy_setopt(m_curlContext, CURLOPT_FOLLOWLOCATION, m_followLocation));

        //执行操作
        CHECK_CURL_RET(curl_easy_perform(m_curlContext));
        //if(formpost)
        //   curl_formfree(formpost);
	    if (m_headers)
		    curl_slist_free_all(m_headers);
	    m_headers = 0;
        return CURLE_OK;

    }
    while(false);
    if(m_headers)
        curl_slist_free_all(m_headers);
    //if(formpost)
    //    curl_formfree(formpost);
    if(m_curlContext)
        curl_easy_cleanup(m_curlContext);
    m_headers       = 0;
    m_curlContext   = 0;
    return retCode;
}

std::string NdHttpClient::GetResponseContent() const
{
    return m_responseContent;
}

std::string NdHttpClient::GetResponseHeaders() const
{
    return m_responseHeaders;
}

std::string NdHttpClient::GetResponseHeaderByKey(const std::string& key)
{
    HttpHeaderIt it = m_responseHeadersMap.find(key);
    if(it != m_responseHeadersMap.end())
    {
        return m_responseHeadersMap[key];
    }
    else
        return std::string("");
}

int NdHttpClient::GetResponseCode() {
    long code = 0;
    curl_easy_getinfo(m_curlContext, CURLINFO_HTTP_CODE, &code);
    return code;
}

void NdHttpClient::ClearRequestState()
{
    m_bDebug                = false;
    m_headers               = 0;
    m_requestHeaders.clear();
    m_responseContent       = "";
    m_responseHeaders       = "";
    m_responseHeadersMap.clear();

    m_postFields            = "";

    m_method                = kGet;
	if (m_curlContext)
		curl_easy_cleanup(m_curlContext);
}

size_t NdHttpClient::OnWriteData(void* buffer, size_t size, size_t nmemb, void* lpVoid)
{
    std::string* str = dynamic_cast<std::string *>((std::string *)lpVoid);
    if(NULL == str || NULL== buffer)
    {
        return -1;
    }

    char *pData = (char *)buffer;
    str->append(pData, size * nmemb);
    return nmemb;
}

size_t NdHttpClient::OnDebug(CURL*, curl_infotype itype, char* pData, size_t size, void*)
{
    if(itype == CURLINFO_TEXT)
    {
        //printf("[TEXT]%s\n", pData);
    }
    else if(itype == CURLINFO_HEADER_IN)
    {
        printf("[HEADER_IN] %s\n", pData);
    }
    else if(itype == CURLINFO_DATA_IN)
    {
        printf("[DATA_IN] %s\n", pData);
    }
    else if(itype == CURLINFO_DATA_OUT)
    {
        printf("[DATA_OUT] %s\n", pData);
    }
    else if(itype == CURLINFO_HEADER_OUT)
    {
        printf("[HEADER_OUT] %s\n", pData);
    }
    return 0;
}

size_t NdHttpClient::OnWriteHeaderData(void* buffer, size_t size, size_t nmemb, void* lpVoid)
{
    NdHttpClient* pThis = (NdHttpClient *)lpVoid;
    if(NULL == pThis || NULL== buffer)
    {
        return -1;
    }

    char *pData = (char *)buffer;
    pThis->m_responseHeaders.append(pData, size * nmemb);

    std::string tmp(pData);
    tmp = tmp.substr(0, tmp.length() - 2);
    pThis->AddToHeaderMap(tmp);
    return nmemb;
}

void NdHttpClient::AddToHeaderMap(const std::string& hdrString)
{
    std::size_t pos         = hdrString.find(":");
    std::string key         = "";
    std::string value       = "";
    if(pos != std::string::npos)
    {
        key                 = hdrString.substr(0, pos);
        value               = hdrString.substr(pos + 1, hdrString.length());
        HttpHeaderIt it     = m_responseHeadersMap.find(key);
        if (it != m_responseHeadersMap.end())
        {
            it->second +=  + ";" +value;
        }
        else
        {
            m_responseHeadersMap.insert(std::make_pair(key, value));
        }
    }
}
