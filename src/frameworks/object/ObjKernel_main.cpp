/// -------------------------------------------------
/// ObjKernel_main.cpp : object框架核心类实现文件
/// -------------------------------------------------
/// Copyright (c) 2015, Wang Yujia <combo.xy@163.com>
/// All rights reserved.
/// -------------------------------------------------

#include "ObjKernel_main.h"
#include "Factory_if.h"


/// -------------------------------------------------
/// 自动构造
/// -------------------------------------------------
void vConstructKernelFunc()
{
    if (!CFrameKernel::sm_pLock)
    {
        CFrameKernel::sm_pLock = DCOP_CreateLock();
    }
}

/// -------------------------------------------------
/// 自动析构
/// -------------------------------------------------
void vReleaseKernelFunc()
{
    if (CFrameKernel::sm_pLock)
    {
        delete CFrameKernel::sm_pLock;
        CFrameKernel::sm_pLock = 0;
    }
}

/// -------------------------------------------------
/// 自动编译
/// -------------------------------------------------
CPPBUILDUNIT_AUTO(vConstructKernelFunc, vReleaseKernelFunc);

/// -------------------------------------------------
/// 全局内核资源和实例
/// -------------------------------------------------
CFrameKernel    CFrameKernel::sm_instance;
objLock *       CFrameKernel::sm_pLock = 0;
void          (*g_onInstanceQueryInterface)(
                        Instance *piThis, 
                        Instance *piRefer, 
                        const char *pinterfaceName, 
                        void *pPara) = 0;
void *          g_onInstanceQueryInterfacePara = 0;
void          (*g_onInstanceRelease)(
                        Instance *piThis, 
                        Instance *piRefer, 
                        void *pPara) = 0;
void *          g_onInstanceReleasePara = 0;
void          (*g_onInstanceGetRef)(
                        Instance *piThis, 
                        Instance ***pppiRefers, 
                        void *pPara) = 0;
void *          g_onInstanceGetRefPara = 0;


/*******************************************************
  函 数 名: objBase::~objBase
  描    述: 虚构实现
  输    入: 
  输    出: 
  返    回: 
  修改记录: 
 *******************************************************/
objBase::~objBase()
{
}

/*******************************************************
  函 数 名: objBase::Enter
  描    述: 实现进入对象基类的全局临界区
  输    入: 
  输    出: 
  返    回: 
  修改记录: 
 *******************************************************/
void objBase::Enter()
{
    if (CFrameKernel::sm_pLock)
    {
        CFrameKernel::sm_pLock->Enter();
    }
}

/*******************************************************
  函 数 名: objBase::Leave
  描    述: 实现退出对象基类的全局临界区
  输    入: 
  输    出: 
  返    回: 
  修改记录: 
 *******************************************************/
void objBase::Leave()
{
    if (CFrameKernel::sm_pLock)
    {
        CFrameKernel::sm_pLock->Leave();
    }
}

/*******************************************************
  函 数 名: objBase::Start
  描    述: 整个应用实例的入口
  输    入: xmlFile - 输入的xml配置文件
  输    出: 
  返    回: 成功或者失败的错误码
  修改记录: 
 *******************************************************/
objBase *objBase::Start(const char *xmlFile)
{
    /////////////////////////////////////////////////
    /// 加载管理器内所有对象
    /////////////////////////////////////////////////
    IManager *piManager = CFrameKernel::sm_instance.Load(xmlFile);
    if (!piManager)
    {
        return NULL;
    }

    /////////////////////////////////////////////////
    /// 初始化管理器内所有对象
    /////////////////////////////////////////////////
    DWORD dwRc = piManager->Init(NULL, 0, 0);
    if (dwRc)
    {
        CHECK_RETCODE(dwRc, STR_FORMAT("System(%d) InitAllObjects Fail(0x%x)!", 
                piManager->GetSystemID(), dwRc));
        (void)piManager->Release();
        return NULL;
    }

    TRACE_LOG(STR_FORMAT("System(%d) InitAllObjects OK!", piManager->GetSystemID()));
    piManager->Dump(PrintToConsole, 0, 0, 0);

    return piManager;
}

/*******************************************************
  函 数 名: objBase::End
  描    述: 整个应用实例的出口
  输    入: 
  输    出: 
  返    回: 
  修改记录: 
 *******************************************************/
void objBase::End(objBase *pBase)
{
    /////////////////////////////////////////////////
    /// 结束管理器内所有对象
    /////////////////////////////////////////////////
    IManager *piManager = (IManager *)pBase;
    if (piManager)
    {
        piManager->Fini();
        DCOP_RELEASE_INSTANCE_REFER(0, piManager);
    }
}

/*******************************************************
  函 数 名: CFrameKernel::CFrameKernel
  描    述: CFrameKernel构造
  输    入: 
  输    出: 
  返    回: 
  修改记录: 
 *******************************************************/
CFrameKernel::CFrameKernel()
{
}

/*******************************************************
  函 数 名: CFrameKernel::~CFrameKernel
  描    述: CFrameKernel析构
  输    入: 
  输    出: 
  返    回: 
  修改记录: 
 *******************************************************/
CFrameKernel::~CFrameKernel()
{
}

/*******************************************************
  函 数 名: CFrameKernel::Load
  描    述: 加载配置文件中的所有对象
  输    入: xmlFile - 输入的xml配置文件
  输    出: 
  返    回: 
  修改记录: 
 *******************************************************/
IManager *CFrameKernel::Load(const char *xmlFile)
{
    DWORD dwRc;
    XMLDocument doc;

    /////////////////////////////////////////////////
    /// 加载xml文件
    /////////////////////////////////////////////////
    dwRc = (DWORD)doc.LoadFile(xmlFile);
    if (dwRc)
    {
        CHECK_RETCODE(dwRc, STR_FORMAT("System('%s') LoadFile Fail!", xmlFile));
        return NULL;
    }

    IFactory *piFactory = IFactory::GetInstance();
    if (piFactory)
    {
        piFactory->Dump(PrintToConsole, 0, 0, 0);
        (void)piFactory->Release();
    }

    /////////////////////////////////////////////////
    /// 获取Xml根节点
    /////////////////////////////////////////////////
    XMLElement *pXmlSystem = doc.RootElement();
    if (!pXmlSystem)
    {
        CHECK_RETCODE(FAILURE, STR_FORMAT("System('%s') LoadFile Fail!", xmlFile));
        return NULL;
    }

    TRACE_LOG(STR_FORMAT("System('%s') LoadFile OK!", xmlFile));

    /////////////////////////////////////////////////
    /// 实例化对象管理器
    /////////////////////////////////////////////////
    CDArray szArgs(DCOP_STRING_ARG_LEM_MAX, DCOP_SYSTEM_ARG_MAX_COUNT);
    char *argv[DCOP_SYSTEM_ARG_MAX_COUNT];
    DWORD argc = GetXmlAttribute(pXmlSystem, szArgs);
    GetArgList(argc, argv, szArgs);
    IManager *piManager = NULL;
    DCOP_CREATE_INSTANCE(IManager, "manager", NULL, argc, argv, piManager);
    if (!piManager)
    {
        CHECK_RETCODE(FAILURE, STR_FORMAT("System('%s') CreateManager Fail!", xmlFile));
        return NULL;
    }

    TRACE_LOG(STR_FORMAT("System('%s') CreateManager OK!", xmlFile));

    /////////////////////////////////////////////////
    /// 创建管理器下面所有对象
    /////////////////////////////////////////////////
    dwRc = CreateAllObjects(piManager, pXmlSystem);
    if (dwRc)
    {
        CHECK_RETCODE(dwRc, STR_FORMAT("System(%d) CreateAllObjects Fail(0x%x)!", 
                piManager->GetSystemID(), dwRc));
        (void)piManager->Release();
        return NULL;
    }

    TRACE_LOG(STR_FORMAT("System(%d) CreateAllObjects OK!", piManager->GetSystemID()));

    return piManager;
}

/*******************************************************
  函 数 名: CFrameKernel::GetXmlAttribute
  描    述: CFrameKernel获取XML元素属性值
  输    入: pXMLElement - XML元素
  输    出: rArgs       - 参数数组
  返    回: 配置项的数量
  修改记录: 
 *******************************************************/
DWORD CFrameKernel::GetXmlAttribute(const XMLElement *pXMLElement, CDArray &rArgs)
{
    DWORD dwArgCount = rArgs.Count();
    const XMLAttribute *pXMLAttribute = pXMLElement->FirstAttribute();
    while (pXMLAttribute)
    {
        char szConfig[DCOP_STRING_ARG_LEM_MAX];
        char szValue[DCOP_STRING_ARG_LEM_MAX];

        /// 配置项名称(为空就不配置了)
        const char *pszName = pXMLAttribute->Name();
        if (!pszName || !(*pszName))
        {
            pXMLAttribute = pXMLAttribute->Next();
            continue;
        }

        (void)snprintf(szConfig, sizeof(szConfig), "-%s", pszName);
        szConfig[sizeof(szConfig) - 1] = '\0';
        if (rArgs.Append(szConfig) != SUCCESS)
        {
            pXMLAttribute = pXMLAttribute->Next();
            continue;
        }

        /// 配置项值(为空就视配置项为开关: 只有名称，而没有值)
        const char *pszValue = pXMLAttribute->Value();
        if (!pszValue || !(*pszValue))
        {
            pXMLAttribute = pXMLAttribute->Next();
            continue;
        }

        (void)snprintf(szValue, sizeof(szValue), "%s", pszValue);
        szValue[sizeof(szValue) - 1] = '\0';
        (void)rArgs.Append(szValue);

        pXMLAttribute = pXMLAttribute->Next();
    }

    if (rArgs.Count() < dwArgCount)
    {
        dwArgCount = 0;
    }
    else
    {
        dwArgCount = rArgs.Count() - dwArgCount;
    }

    return dwArgCount;
}

/*******************************************************
  函 数 名: CFrameKernel::GetXmlChildValue
  描    述: CFrameKernel获取XML元素下的子元素值(只限DWORD)
  输    入: pXMLElement - XML元素
  输    出: rArgs       - 参数数组
  返    回: 子元素的数量
  修改记录: 
 *******************************************************/
DWORD CFrameKernel::GetXmlChildValue(const XMLElement *pXMLElement, CDArray &rArgs)
{
    DWORD dwArgCount = rArgs.Count();
    const XMLElement *pXMLChildElement = pXMLElement->FirstChildElement();
    while (pXMLChildElement)
    {
        char szConfig[DCOP_STRING_ARG_LEM_MAX];
        char szValue[DCOP_STRING_ARG_LEM_MAX];

        /// 子元素名称(为空就不配置了)
        const char *pszName = pXMLChildElement->Name();
        if (!pszName || !(*pszName))
        {
            pXMLChildElement = pXMLChildElement->NextSiblingElement();
            continue;
        }

        (void)snprintf(szConfig, sizeof(szConfig), "-%s", pszName);
        szConfig[sizeof(szConfig) - 1] = '\0';
        if (rArgs.Append(szConfig) != SUCCESS)
        {
            pXMLChildElement = pXMLChildElement->NextSiblingElement();
            continue;
        }

        /// 配置项值(为空就视配置项为开关: 只有名称，而没有值)
        const char *pszValue = pXMLChildElement->GetText();
        if (!pszValue || !(*pszValue))
        {
            pXMLChildElement = pXMLChildElement->NextSiblingElement();
            continue;
        }

        (void)snprintf(szValue, sizeof(szValue), "%s", pszValue);
        szValue[sizeof(szValue) - 1] = '\0';
        (void)rArgs.Append(szValue);

        pXMLChildElement = pXMLChildElement->NextSiblingElement();
    }

    if (rArgs.Count() < dwArgCount)
    {
        dwArgCount = 0;
    }
    else
    {
        dwArgCount = rArgs.Count() - dwArgCount;
    }

    return dwArgCount;
}

/*******************************************************
  函 数 名: CFrameKernel::GetArgList
  描    述: CFrameKernel获取char **argv这种参数形式
  输    入: crArgs      - 参数数组(最好把argc置为和crArgs数量相等)
  输    出: argv        - 参数列表
  返    回: 子元素的数量
  修改记录: 
 *******************************************************/
void CFrameKernel::GetArgList(DWORD argc, char **argv, const CDArray &crArgs)
{
    for (DWORD i = 0; i < argc; ++i)
    {
        char *pStr = (char *)crArgs.Pos(i);
        if (!pStr)
        {
            break;
        }

        argv[i] = pStr;
    }
}

/*******************************************************
  函 数 名: CFrameKernel::CreateAllObjects
  描    述: CFrameKernel创建所有XML描述中的对象
  输    入: piManager   - 管理器
            pXMLElement - XML元素
  输    出: 
  返    回: 成功或者失败的错误码
  修改记录: 
 *******************************************************/
DWORD CFrameKernel::CreateAllObjects(IManager *piManager, const XMLElement *pXMLElement)
{
    if (!piManager) return FAILURE;

    const XMLElement *pXmlObjects = pXMLElement->FirstChildElement("objects");
    if (!pXmlObjects)
    {
        return FAILURE;
    }

    const XMLElement *pXmlObject = pXmlObjects->FirstChildElement();
    DWORD dwRc = SUCCESS;
    while (pXmlObject)
    {
        CDArray szArgs(DCOP_STRING_ARG_LEM_MAX, DCOP_OBJECT_ARG_MAX_COUNT);
        char *argv[DCOP_OBJECT_ARG_MAX_COUNT];
        DWORD argc = 0;
        argc  = GetXmlAttribute (pXmlObject, szArgs);
        argc += GetXmlChildValue(pXmlObject, szArgs);
        GetArgList(argc, argv, szArgs);

        const char *szName = pXmlObject->Attribute("ins");
        if (!szName) szName = pXmlObject->Attribute("name");
        IObject *piObject = 0;
        DCOP_CREATE_INSTANCE(IObject, szName, piManager, argc, argv, piObject);
        if (!piObject)
        {
            dwRc = FAILURE;
            CHECK_RETCODE(dwRc, STR_FORMAT("Create Object: '%s'|%s Fail(0x%x)!", 
                    szName, pXmlObject->Attribute("id"), dwRc));
            pXmlObject = pXmlObject->NextSiblingElement();
            continue;
        }

        DWORD dwRcTmp = piManager->InsertObject(piObject);
        if (dwRcTmp != SUCCESS)
        {
            CHECK_RETCODE(dwRcTmp, STR_FORMAT("Insert Object: '%s'|%d Fail(0x%x)!", 
                    piObject->Name(), piObject->ID(), dwRcTmp));
            DCOP_RELEASE_INSTANCE_REFER(piManager, piObject);
            dwRc |= dwRcTmp;
            pXmlObject = pXmlObject->NextSiblingElement();
            continue;
        }
    
        pXmlObject = pXmlObject->NextSiblingElement();
    }

    return dwRc;
}

