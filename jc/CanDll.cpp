// tcCanDll.cpp : Defines the initialization routines for the DLL.
//

#include "stdafx.h"
#include <afxdllx.h>
#include "CanDll.h"

#include "tcTimerCan.h"
#include "tcCanNonSession.h"
#include "tcCanNI.h"
#include "tcCanVector.h"
#include "tcCanAIXIA.h"
#include "tcCanNsi.h"
#include "tcCanJCAE.h"
#include "tcCanADLINK.h"
#include "tcCanTestUUDT.h"
#include "Version.h"

#include "ToolBox.h"

#ifdef _DEBUG
#define new DEBUG_NEW
#undef THIS_FILE
static char THIS_FILE[] = __FILE__;
#endif

static AFX_EXTENSION_MODULE tcCanDllDLL = { NULL, NULL };

static tcCanSession* GetCanCard(tcParamCan* pParamCan,CString& Err);


extern "C" int APIENTRY
DllMain(HINSTANCE hInstance, DWORD dwReason, LPVOID lpReserved)
{
	// Remove this if you use lpReserved
	UNREFERENCED_PARAMETER(lpReserved);

	if (dwReason == DLL_PROCESS_ATTACH)
	{
		TRACE0("CANDLL.DLL Initializing!\n");
		
		// Extension DLL one-time initialization
		if (!AfxInitExtensionModule(tcCanDllDLL, hInstance))
			return 0;

		// Insert this DLL into the resource chain
		// NOTE: If this Extension DLL is being implicitly linked to by
		//  an MFC Regular DLL (such as an ActiveX Control)
		//  instead of an MFC application, then you will want to
		//  remove this line from DllMain and put it in a separate
		//  function exported from this Extension DLL.  The Regular DLL
		//  that uses this Extension DLL should then explicitly call that
		//  function to initialize this Extension DLL.  Otherwise,
		//  the CDynLinkLibrary object will not be attached to the
		//  Regular DLL's resource chain, and serious problems will
		//  result.

		new CDynLinkLibrary(tcCanDllDLL);
	}
	else if (dwReason == DLL_PROCESS_DETACH)
	{
		TRACE0("CANDLL.DLL Terminating!\n");
		// Terminate the library before destructors are called
		AfxTermExtensionModule(tcCanDllDLL);
	}
	return 1;   // ok
}

//////////////////////////////////////////////////////////////////////
// Version
//////////////////////////////////////////////////////////////////////
U16 tcCanDll::GetVersion()
{
  CVersion Version;
  return Version.LoadNumber(tcCanDllDLL.hModule);
}

//////////////////////////////////////////////////////////////////////
// CanCard list
//////////////////////////////////////////////////////////////////////
#define cMaxCanCardNameTab    21

static char CanCardNameTab[cMaxCanCardNameTab][MAX_PATH] =
  {
    cCanCardAUTOMATIC,
    cCanCardNSIPort0,
    cCanCardNSIPort1,
    cCanCardVECTORCanAC2PciXPort0,
    cCanCardVECTORCanAC2PciXPort1,
    cCanCardVECTORCanCardXPort0,
    cCanCardVECTORCanCardXPort1,
    cCanCardVECTORCanCardXLPort0,
    cCanCardVECTORCanCardXLPort1,
    cCanCardVECTORCanBoardXLPort0,
    cCanCardVECTORCanBoardXLPort1,
    cCanCardVECTORCanCaseXLPort0,
    cCanCardVECTORCanCaseXLPort1,
    cCanCardNIPort0,
    cCanCardNIPort1,
    cCanCardJCIPort0,
    cCanCardAIXIACom1Port0,
    cCanCardAIXIACom2Port0,
    cCanCardADLINKPort0,
    cCanCardADLINKPort1,
    cCanCardVIRTUAL,
  };


U8 tcCanDll::GetCanCardListAvailable(U8 Num,char* CanCardName)
{
  if (CanCardName != NULL)
  {
    strcpy(CanCardName,CanCardNameTab[Num]);
  }
  return cMaxCanCardNameTab;
}

//////////////////////////////////////////////////////////////////////
// Construction/Destruction
//////////////////////////////////////////////////////////////////////
tcCanDll::tcCanDll(tcParamCan* pParamCan)
{
  m_pParam = NULL;
  // Create Can session
  int       CardNumber = 0;
  CString   Err;
  CString   FullError;
  CArray<CString,CString> CanCardList;

  // Search Can card
  if (strcmp(pParamCan->CanCardName,cCanCardAUTOMATIC) == 0)
  {
    char Buffer[MAX_PATH];
    for(U8 i=0;i<GetCanCardListAvailable(0,NULL);i++)
    {
      GetCanCardListAvailable(i,Buffer);
      // Remove ADLink card in Automatic research because it open DialogBox
      if (strncmp(pParamCan->CanCardName,cCanCardADLINK,sizeof(cCanCardADLINK)-1))
      {
        CanCardList.Add(Buffer);
      }
    }
  }
  else
  {
    CanCardList.Add(pParamCan->CanCardName);
  }
  while((CardNumber < CanCardList.GetSize()) && (m_pParam == NULL))
  {
    strcpy(pParamCan->CanCardName,CanCardList[CardNumber]);
    if ((strcmp(pParamCan->CanCardName,cCanCardVIRTUAL) != 0)     // Pass cCanCardVIRTUAL if not first
      ||(CardNumber == 0))
    {
      if (strcmp(pParamCan->CanCardName,cCanCardAUTOMATIC) != 0)  // Pass cCanCardAUTOMATIC
      { 
        m_pParam = GetCanCard(pParamCan,Err);
        if (m_pParam == NULL)
        {
          FullError += Err;
          FullError += '\n';
        }
      }
    }
    CardNumber++;
  }
  if((pParamCan->FCallBackMsg != NULL) && (m_pParam == NULL)) // Can card not found
  {
    pParamCan->FCallBackMsg(cErrorMsg,FullError,pParamCan->ParamCallBackMsg);
  }
}

tcCanDll::~tcCanDll()
{
  if (m_pParam != NULL)
  {
    delete ((tcCanSession*)m_pParam);
  }
}

static void CallBackMsgTmp(tMsgType MsgType,const char* pMsg,void* Param)
{
  // Param is CString for possible error
  *((CString*)Param) = pMsg;
}

tcCanSession* GetCanCard(tcParamCan* pParamCan,CString& Err)
{
  // Change callback msg
  CString CanCardError;
  tpfCallBackCanMsg FCallBackMsg      = pParamCan->FCallBackMsg;
  void*             ParamCallBackMsg  = pParamCan->ParamCallBackMsg;
  pParamCan->FCallBackMsg             = CallBackMsgTmp;
  pParamCan->ParamCallBackMsg         = &CanCardError;

  tcCanSession* pCanSession = NULL;
  tcCanUUDT*    pCanUUDT    = NULL;

  if (!strncmp(pParamCan->CanCardName,cCanCardNSI,sizeof(cCanCardNSI)-1))
  {
    if (pParamCan->CanCardName[sizeof(cCanCardNSI)-1] == '(')
    {
      U32 Nb;
      CString str(pParamCan->CanCardName);
      if (AscToNb(str.Mid(sizeof(cCanCardNSI)),&Nb))
      {
        pParamCan->Port = (U16)Nb;
      }
    }
    pCanUUDT = new tcCanNsi(&gTimerCan,pParamCan);
  }
  else if (!strncmp(pParamCan->CanCardName,cCanCardVECTOR,sizeof(cCanCardVECTOR)-1))
  {
    if (pParamCan->CanCardName[sizeof(cCanCardVECTOR)-1] == '(')
    {
      U32 Nb;
      CString str(pParamCan->CanCardName);
      AscToNb(str.Mid(sizeof(cCanCardVECTOR)),&Nb);
      pParamCan->Port = (U16)Nb;
    }
    pCanUUDT = new tcCanVector(&gTimerCan,pParamCan);
  }
  else if (!strncmp(pParamCan->CanCardName,cCanCardNI,sizeof(cCanCardNI)-1))
  {
    if (pParamCan->CanCardName[sizeof(cCanCardNI)-1] == '(')
    {
      U32 Nb;
      CString str(pParamCan->CanCardName);
      AscToNb(str.Mid(sizeof(cCanCardNI)),&Nb);
      pParamCan->Port = (U16)Nb;
    }
    pCanUUDT = new tcCanNI(&gTimerCan,pParamCan);
  }
  else if (!strncmp(pParamCan->CanCardName,cCanCardJCI,sizeof(cCanCardJCI)-1))
  {
    if (pParamCan->CanCardName[sizeof(cCanCardJCI)-1] == '(')
    {
      U32 Nb;
      CString str(pParamCan->CanCardName);
      AscToNb(str.Mid(sizeof(cCanCardJCI)),&Nb);
      pParamCan->Port = (U16)Nb;
    }
    pCanUUDT = new tcCanJCAE(&gTimerCan,pParamCan);
  }
  else if (!strncmp(pParamCan->CanCardName,cCanCardAIXIA,sizeof(cCanCardAIXIA)-1))
  {
    if (pParamCan->CanCardName[sizeof(cCanCardAIXIA)-1] == '(')
    {
      U32 Nb;
      CString str(pParamCan->CanCardName);
      AscToNb(str.Mid(sizeof(cCanCardAIXIA)),&Nb);
      pParamCan->Port = (U16)Nb;
    }
    pCanUUDT = new tcCanAIXIA(&gTimerCan,pParamCan);
  }
  else if (!strcmp(pParamCan->CanCardName,cCanCardVIRTUAL))
  {
    if (pParamCan->CanCardName[sizeof(cCanCardVIRTUAL)-1] == '(')
    {
      U32 Nb;
      CString str(pParamCan->CanCardName);
      AscToNb(str.Mid(sizeof(cCanCardVIRTUAL)),&Nb);
      pParamCan->Port = (U16)Nb;
    }
    pCanUUDT = new tcCanTestUUDT(&gTimerCan,pParamCan);
  }
  else if (!strncmp(pParamCan->CanCardName,cCanCardADLINK,sizeof(cCanCardADLINK)-1))
  {
    if (pParamCan->CanCardName[sizeof(cCanCardADLINK)-1] == '(')
    {
      U32 Nb;
      CString str(pParamCan->CanCardName);
      AscToNb(str.Mid(sizeof(cCanCardADLINK)),&Nb);
      pParamCan->Port = (U16)Nb;
    }
    pCanUUDT = new tcCanADLINK(&gTimerCan,pParamCan);
  }

  // Set old callback msg
  pParamCan->FCallBackMsg             = FCallBackMsg;
  pParamCan->ParamCallBackMsg         = ParamCallBackMsg;

  if (pCanUUDT != NULL)
  {
    if (!CanCardError.IsEmpty())
    {
      Err = CanCardError;
      delete pCanUUDT;
    }
    else
    {
      pCanSession = new tcCanNonSession(&gTimerCan,pParamCan,pCanUUDT);
    }
  }
  else
  {
    Err.Format("Unknown can card name: \"%s\"",pParamCan->CanCardName);
  }
  return pCanSession;
}

//////////////////////////////////////////////////////////////////////
// Can timer
//////////////////////////////////////////////////////////////////////
void tcCanDll::InitializeCanTimer()
{
  gTimerCan.Initialiser();
}

void tcCanDll::ExecuteCanTimer(U32 Date)
{
  gTimerCan.Executer(Date);
}

U32 tcCanDll::GetCanTimerDate()
{
  return gTimerCan.GetDate();
}

//////////////////////////////////////////////////////////////////////
// CallBackMsg
//////////////////////////////////////////////////////////////////////
void tcCanDll::SetCallBackMsg(tpfCallBackCanMsg FCallBackMsg,void* ParamCallBackMsg)
{
  ASSERT(m_pParam != NULL);
  ((tcCanSession*)m_pParam)->SetCallBackMsg(FCallBackMsg,ParamCallBackMsg);
}

//////////////////////////////////////////////////////////////////////
// Can session
//////////////////////////////////////////////////////////////////////
//---- Session object ----
U8 tcCanDll::AddCanObject(tObjetCanSession* pObjetCan)
{
  ASSERT(m_pParam != NULL);
  return ((tcCanSession*)m_pParam)->CreerObjet(pObjetCan);
}

void tcCanDll::RemoveCanObjet(tObjetCanSession* pObjetCan)
{
  ASSERT(m_pParam != NULL);
  ((tcCanSession*)m_pParam)->DetruireObjet(pObjetCan);
}

U8 tcCanDll::Connect(tObjetCanSession* pObjetCan)
{
  ASSERT(m_pParam != NULL);
  return ((tcCanSession*)m_pParam)->Connect(pObjetCan);
}

U8 tcCanDll::Deconnect(tObjetCanSession* pObjetCan)
{
  ASSERT(m_pParam != NULL);
  return ((tcCanSession*)m_pParam)->Deconnect(pObjetCan);
}

U8 tcCanDll::Req(tObjetCanSession* pObjetCan,tMsgCan* pMsg)
{
  ASSERT(m_pParam != NULL);
  return ((tcCanSession*)m_pParam)->Req(pObjetCan,pMsg);
}


//---- ASDT object ----
U8 tcCanDll::AddCanObject(tObjetCanAS* pObjetCan)
{
  ASSERT(m_pParam != NULL);
  return ((tcCanSession*)m_pParam)->CreerObjet(pObjetCan);
}

void tcCanDll::RemoveCanObjet(tObjetCanAS* pObjetCan)
{
  ASSERT(m_pParam != NULL);
  ((tcCanSession*)m_pParam)->DetruireObjet(pObjetCan);
}

U8 tcCanDll::Req(tObjetCanAS* pObjetCan,tMsgCan* pMsg)
{
  ASSERT(m_pParam != NULL);
  return ((tcCanSession*)m_pParam)->Req(pObjetCan,pMsg);
}

//---- USDT object ----
U8 tcCanDll::AddCanObject(tObjetCanUS* pObjetCan)
{
  ASSERT(m_pParam != NULL);
  return ((tcCanSession*)m_pParam)->CreerObjet(pObjetCan);
}

void tcCanDll::RemoveCanObjet(tObjetCanUS* pObjetCan)
{
  ASSERT(m_pParam != NULL);
  ((tcCanSession*)m_pParam)->DetruireObjet(pObjetCan);
}

U8 tcCanDll::Req(tObjetCanUS* pObjetCan,tMsgCan* pMsg)
{
  ASSERT(m_pParam != NULL);
  return ((tcCanSession*)m_pParam)->Req(pObjetCan,pMsg);
}

//---- UUDT object ----
U8 tcCanDll::AddCanObject(tObjetCanUU* pObjetCan)
{
  ASSERT(m_pParam != NULL);
  return ((tcCanSession*)m_pParam)->CreerObjet(pObjetCan);
}

void tcCanDll::RemoveCanObjet(tObjetCanUU* pObjetCan)
{
  ASSERT(m_pParam != NULL);
  ((tcCanSession*)m_pParam)->DetruireObjet(pObjetCan);
}

U8 tcCanDll::Req(tObjetCanUU* pObjetCan,tTrameCan* pTrame,U8 EnvoiImmediat)
{
  ASSERT(m_pParam != NULL);
  return ((tcCanSession*)m_pParam)->Req(pObjetCan,pTrame,EnvoiImmediat);
}

U8 tcCanDll::ActivateReqPeriodical(tObjetCanUU* pObjetCan,U8 Active)
{
  ASSERT(m_pParam != NULL);
  return ((tcCanSession*)m_pParam)->ActiverReqPeriodique(pObjetCan,Active);
}

U8 tcCanDll::Control(tCanControl CanControl,void* Param)
{
  return ((tcCanSession*)m_pParam)->Control(CanControl,Param);
}
