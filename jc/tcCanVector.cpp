// tcCanVector.cpp: implementation of the tcCanVector class.
//
//////////////////////////////////////////////////////////////////////
#include "stdafx.h"
#include "Config.h"
#ifdef CANAL_CAN      // Le canal can est utilisé

#include "tcCanVector.h"

#ifdef _DEBUG
#undef THIS_FILE
static char THIS_FILE[]=__FILE__;
#define new DEBUG_NEW
#endif

//////////////////////////////////////////////////////////////////////
// Construction/Destruction
//////////////////////////////////////////////////////////////////////
int                       tcCanVector::m_NbCarteChargee = 0;
HINSTANCE                 tcCanVector::m_hLib = NULL;

NCDOPENDRIVER             tcCanVector::ncdOpenDriver;
NCDGETCHANNELMASK         tcCanVector::ncdGetChannelMask;
NCDOPENPORT               tcCanVector::ncdOpenPort;
NCDSETCHANNELMODE         tcCanVector::ncdSetChannelMode;
NCDSETCHANNELBITRATE      tcCanVector::ncdSetChannelBitrate;
NCDSETCHANNELACCEPTANCE   tcCanVector::ncdSetChannelAcceptance;
NCDSETNOTIFICATION        tcCanVector::ncdSetNotification;
NCDTRANSMIT               tcCanVector::ncdTransmit;
NCDRECEIVE1               tcCanVector::ncdReceive1;
NCDACTIVATECHANNEL        tcCanVector::ncdActivateChannel;
NCDDEACTIVATECHANNEL      tcCanVector::ncdDeactivateChannel;
NCDCLOSEPORT              tcCanVector::ncdClosePort;
NCDCLOSEDRIVER            tcCanVector::ncdCloseDriver;
NCDGETEVENTSTRING         tcCanVector::ncdGetEventString;
NCDFLUSHRECEIVEQUEUE      tcCanVector::ncdFlushReceiveQueue;
NCDFLUSHTRANSMITQUEUE     tcCanVector::ncdFlushTransmitQueue;
NCDSETCHANNELTRANSCEIVER  tcCanVector::ncdSetChannelTransceiver;
NCDSETCHANNELOUTPUT       tcCanVector::ncdSetChannelOutput;

tcCanVector::tcCanVector(tcTimerCan* pTimerCan,tcParamCan* pParamCan):tcCanUUDT(pTimerCan,pParamCan)
{
  ASSERT(pParamCan != NULL);
  if (!strcmp(pParamCan->CanCardName,cCanCardVECTORCanAC2PciXPort0))
  {
    pParamCan->Port = 0x0600;
  }
  else if (!strcmp(pParamCan->CanCardName,cCanCardVECTORCanAC2PciXPort1))
  {
    pParamCan->Port = 0x0601;
  }
  else if (!strcmp(pParamCan->CanCardName,cCanCardVECTORCanCardXPort0))
  {
    pParamCan->Port = 0x0200;
  }
  else if (!strcmp(pParamCan->CanCardName,cCanCardVECTORCanCardXPort1))
  {
    pParamCan->Port = 0x0201;
  }
  else if (!strcmp(pParamCan->CanCardName,cCanCardVECTORCanCardXLPort0))
  {
    pParamCan->Port = 0x0F00;
  }
  else if (!strcmp(pParamCan->CanCardName,cCanCardVECTORCanCardXLPort1))
  {
    pParamCan->Port = 0x0F01;
  }
  else if (!strcmp(pParamCan->CanCardName,cCanCardVECTORCanBoardXLPort0))
  {
    pParamCan->Port = 0x1900;
  }
  else if (!strcmp(pParamCan->CanCardName,cCanCardVECTORCanBoardXLPort1))
  {
    pParamCan->Port = 0x1901;
  }
  else if (!strcmp(pParamCan->CanCardName,cCanCardVECTORCanCaseXLPort0))
  {
    pParamCan->Port = 0x1500;
  }
  else if (!strcmp(pParamCan->CanCardName,cCanCardVECTORCanCaseXLPort1))
  {
    pParamCan->Port = 0x1501;
  }
  memcpy(&m_ParamCan,pParamCan,sizeof(m_ParamCan));

  m_Arret=cFaux;
  m_ThreadID=0;
  int gHwType                 = pParamCan->Port>>8;           // HWTYPE
  int gHwChannel              = (pParamCan->Port&0xFF)%2;     // 2 ports par carte
  int gHwIndex                = (pParamCan->Port&0xFF)/2;     // N° de carte
  Vstatus Statut;
  VsetAcceptance acc;
  Vaccess gPermissionMask     = 0;
  m_gChannelMask              = 0;
  m_gPortHandle               = INVALID_PORTHANDLE;
//  m_iErrorFrameTimeoutTimerID = cNumeroTempoInvalide;

  m_NbCarteChargee++;

  if (m_hLib==NULL)   // On charge la DLL dynamiquement
  {
    char  Buffer[1024];
    GetSystemDirectory(Buffer,1024);
    CString str;
    str = "vCand32.dll";
    m_hLib = LoadLibrary(str);
    if (m_hLib==NULL)
    {
      str.Format("tcCanVector::tcCanVector> Loading error: %s\\vCand32.dll",Buffer);
      SetMsg(cErrorMsg,str);
      return;
    }
    if ((ncdOpenDriver = (NCDOPENDRIVER)GetProcAddress(m_hLib,  _T("ncdOpenDriver"))) == NULL)
    {
      SetMsg(cErrorMsg,"tcCanVector::tcCanVector> Address loading failed: ncdOpenDriver");
      return;
    }
    if (((FARPROC&)ncdGetChannelMask = GetProcAddress(m_hLib,  _T("ncdGetChannelMask"))) == NULL)
    {
      SetMsg(cErrorMsg,"tcCanVector::tcCanVector> Address loading failed: ncdGetChannelMask");
      return;
    }
    if (((FARPROC&)ncdOpenPort = GetProcAddress(m_hLib,  _T("ncdOpenPort"))) == NULL)
    {
      SetMsg(cErrorMsg,"tcCanVector::tcCanVector> Address loading failed: ncdOpenPort");
      return;
    }
    if (((FARPROC&)ncdSetChannelMode = GetProcAddress(m_hLib,  _T("ncdSetChannelMode"))) == NULL)
    {
      SetMsg(cErrorMsg,"tcCanVector::tcCanVector> Address loading failed: ncdSetChannelMode");
      return;
    }
    if (((FARPROC&)ncdSetChannelBitrate = GetProcAddress(m_hLib,  _T("ncdSetChannelBitrate"))) == NULL)
    {
      SetMsg(cErrorMsg,"tcCanVector::tcCanVector> Address loading failed: ncdSetChannelBitrate");
      return;
    }
    if (((FARPROC&)ncdSetChannelAcceptance = GetProcAddress(m_hLib,  _T("ncdSetChannelAcceptance"))) == NULL)
    {
      SetMsg(cErrorMsg,"tcCanVector::tcCanVector> Address loading failed: ncdSetChannelAcceptance");
      return;
    }
    if (((FARPROC&)ncdSetNotification = GetProcAddress(m_hLib,  _T("ncdSetNotification"))) == NULL)
    {
      SetMsg(cErrorMsg,"tcCanVector::tcCanVector> Address loading failed: ncdSetNotification");
      return;
    }
    if (((FARPROC&)ncdTransmit = GetProcAddress(m_hLib,  _T("ncdTransmit"))) == NULL)
    {
      SetMsg(cErrorMsg,"tcCanVector::tcCanVector> Address loading failed: ncdTransmit");
      return;
    }
    if (((FARPROC&)ncdReceive1 = GetProcAddress(m_hLib,  _T("ncdReceive1"))) == NULL)
    {
      SetMsg(cErrorMsg,"tcCanVector::tcCanVector> Address loading failed: ncdReceive1");
      return;
    }
    if (((FARPROC&)ncdActivateChannel = GetProcAddress(m_hLib,  _T("ncdActivateChannel"))) == NULL)
    {
      SetMsg(cErrorMsg,"tcCanVector::tcCanVector> Address loading failed: ncdActivateChannel");
      return;
    }
    if (((FARPROC&)ncdDeactivateChannel= GetProcAddress(m_hLib,  _T("ncdDeactivateChannel"))) == NULL)
    {
      SetMsg(cErrorMsg,"tcCanVector::tcCanVector> Address loading failed: ncdDeactivateChannel");
      return;
    }
    if (((FARPROC&)ncdClosePort = GetProcAddress(m_hLib,  _T("ncdClosePort"))) == NULL)
    {
      SetMsg(cErrorMsg,"tcCanVector::tcCanVector> Address loading failed: ncdClosePort");
      return;
    }
    if (((FARPROC&)ncdCloseDriver = GetProcAddress(m_hLib,  _T("ncdCloseDriver"))) == NULL)
    {
      SetMsg(cErrorMsg,"tcCanVector::tcCanVector> Address loading failed: ncdCloseDriver");
      return;
    }
    if (((FARPROC&)ncdGetEventString = GetProcAddress(m_hLib,  _T("ncdGetEventString"))) == NULL)
    {
      SetMsg(cErrorMsg,"tcCanVector::tcCanVector> Address loading failed: ncdGetEventString");
      return;
    }
    if (((FARPROC&)ncdFlushReceiveQueue = GetProcAddress(m_hLib,  _T("ncdFlushReceiveQueue"))) == NULL)
    {
      SetMsg(cErrorMsg,"tcCanVector::tcCanVector> Address loading failed: ncdFlushReceiveQueue");
      return;
    }
    if (((FARPROC&)ncdFlushTransmitQueue = GetProcAddress(m_hLib,  _T("ncdFlushTransmitQueue"))) == NULL)
    {
      SetMsg(cErrorMsg,"tcCanVector::tcCanVector> Address loading failed: ncdFlushTransmitQueue");
      return;
    }
    if (((FARPROC&)ncdSetChannelTransceiver = GetProcAddress(m_hLib,  _T("ncdSetChannelTransceiver"))) == NULL)
    {
      SetMsg(cErrorMsg,"tcCanVector::tcCanVector> Address loading failed: ncdSetChannelTransceiver");
      return;
    }
    if (((FARPROC&)ncdSetChannelOutput = GetProcAddress(m_hLib,  _T("ncdSetChannelOutput"))) == NULL)
    {
      SetMsg(cErrorMsg,"tcCanVector::tcCanVector> Address loading failed: ncdSetChannelOutput");
      return;
    }
  }

  //  On initialise le Driver
  Statut = ncdOpenDriver();
  if (Statut!=VSUCCESS)
  {
    char Buffer[256];
    sprintf(Buffer,"tcCanVector::tcCanVector> Can driver failed (%04X)",Statut);
    SetMsg(cErrorMsg,Buffer);
    return;
  }
  m_CartePresente = cVrai;

  // On cherche le masque du canal Cancardx d'index 0 et de canal Port
  m_gChannelMask = ncdGetChannelMask(gHwType,gHwIndex,gHwChannel);
  if (m_gChannelMask==0) 
  {
    char Buffer[256];
    sprintf(Buffer,"tcCanVector::tcCanVector> %s not found",m_ParamCan.CanCardName);
    SetMsg(cErrorMsg,Buffer);
    return;
  }
  gPermissionMask = m_gChannelMask;
  // On ouvre le port
  Statut = ncdOpenPort(&m_gPortHandle,"JCAE",m_gChannelMask,m_gChannelMask,&gPermissionMask,pParamCan->QueueRead);
  if (Statut!=VSUCCESS)
  {
    char Buffer[256];
    sprintf(Buffer,"tcCanVector::tcCanVector> Can port failed (%04X)",Statut);
    SetMsg(cErrorMsg,Buffer);
    return;
  }
  if (gPermissionMask!=m_gChannelMask)
  {
    SetMsg(cErrorMsg,"tcCanVector::tcCanVector> Can card configuration unauthorized");
    return;
  }
  // On configure la vitesse
  Statut = ncdSetChannelBitrate(m_gPortHandle,m_gChannelMask,pParamCan->BaudRate);
  if (Statut!=VSUCCESS)
  {
    char Buffer[256];
    sprintf(Buffer,"tcCanVector::tcCanVector> Baudrate configuration failed (%04X)",Statut);
    SetMsg(cErrorMsg,Buffer);
    return;
  }
  // Inhiber les notifications TX et TXRQ
  Statut = ncdSetChannelMode(m_gPortHandle,m_gChannelMask,0,0);
  if (Statut!=VSUCCESS)
  {
    char Buffer[256];
    sprintf(Buffer,"tcCanVector::tcCanVector> Tx and TXRQ disabling failed (%04X)",Statut);
    SetMsg(cErrorMsg,Buffer);
    return;
  }
  // On configure les masques d'acceptances
  acc.mask = pParamCan->MaskStd; // relevant=1
  acc.code = pParamCan->CodeStd;
  Statut = ncdSetChannelAcceptance(m_gPortHandle,m_gChannelMask,&acc);
  if (Statut!=VSUCCESS)
  {
    char Buffer[256];
    sprintf(Buffer,"tcCanVector::tcCanVector> STD Mask id configuration failed (%04X)",Statut);
    SetMsg(cErrorMsg,Buffer);
    return;
  }
  acc.mask = 0x80000000 | pParamCan->MaskXtd; // relevant=1
  acc.code = 0x80000000 | pParamCan->CodeXtd;
  Statut = ncdSetChannelAcceptance(m_gPortHandle,m_gChannelMask,&acc);
  if (Statut!=VSUCCESS)
  {
    char Buffer[256];
    sprintf(Buffer,"tcCanVector::tcCanVector> XTD Mask id configuration failed (%04X)",Statut);
    SetMsg(cErrorMsg,Buffer);
    return;
  }
  // On vide la queue de réception
  Statut = ncdFlushReceiveQueue(m_gPortHandle);
  if (Statut!=VSUCCESS)
  {
    char Buffer[256];
    sprintf(Buffer,"tcCanVector::tcCanVector> Reception queue erasing failed (%04X)",Statut);
    SetMsg(cErrorMsg,Buffer);
    return;
  }
  // On vide la queue de transmission
  Statut = ncdFlushTransmitQueue(m_gPortHandle,m_gChannelMask);
  if (Statut!=VSUCCESS)
  {
    char Buffer[256];
    sprintf(Buffer,"tcCanVector::tcCanVector> Emission queue erasing failed (%04X)",Statut);
    SetMsg(cErrorMsg,Buffer);
    return;
  }
  // On crée l'évènement pour recevoir
  m_gEventHandle = CreateEvent(NULL, FALSE, FALSE, NULL);
  Statut = ncdSetNotification(m_gPortHandle,(unsigned long*)&m_gEventHandle, 1);
  if (Statut!=VSUCCESS)
  {
    char Buffer[256];
    sprintf(Buffer,"tcCanVector::tcCanVector> Reception event configuration failed (%04X)",Statut);
    SetMsg(cErrorMsg,Buffer);
    return;
  }
  // On initialise le corps du thread
  m_hThread = CreateThread( NULL,0,CorpsThread,(LPVOID)this,0,&m_ThreadID);
  if(m_hThread==INVALID_HANDLE_VALUE)
  {
    SetMsg(cErrorMsg,"tcCanVector::tcCanVector> Reception thread creation failed");
    return;
  }
  // On active le canal
  Statut = ncdActivateChannel(m_gPortHandle,m_gChannelMask);
  if (Statut!=VSUCCESS)
  {
    char Buffer[256];
    if(Statut == VERR_PORT_IS_OFFLINE)
    {
        sprintf(Buffer,"tcCanVector (Error : %04X) > !!!!!!!!!!!!!!!!! Please, Plug the CAN Cable !!!!!!!!!!!!!!!!!",Statut);
    }
    else
    {
        sprintf(Buffer,"tcCanVector::tcCanVector> Channel activation failed (%04X)",Statut);
    }
    SetMsg(cErrorMsg,Buffer);
    return;
  }

  m_StatutReq = VSUCCESS;
  m_InitOk=cVrai;

#ifndef DISABLE_SAD
  if (!strcmp(pParamCan->CanCardName,cCanCardVECTORCanAC2PciXPort0))
  {
    m_SADName = "VectorCanAC2PciX0";
  }
  else if (!strcmp(pParamCan->CanCardName,cCanCardVECTORCanAC2PciXPort1))
  {
    m_SADName = "VectorCanAC2PciX1";
  }
  else if (!strcmp(pParamCan->CanCardName,cCanCardVECTORCanCardXPort0))
  {
    m_SADName = "VectorCanCardX0";
  }
  else if (!strcmp(pParamCan->CanCardName,cCanCardVECTORCanCardXPort1))
  {
    m_SADName = "VectorCanCardX1";
  }
  else
  {
    m_SADName.Format("Vector%04X",pParamCan->Port);
  }
  char Name[cSADMaxNameLength];
  sprintf(Name,"CanUU%s",m_SADName);
  m_SADAdrFrame.SADConnect(Name);
#endif
}

tcCanVector::~tcCanVector()
{
#ifndef DISABLE_SAD
  m_SADAdrFrame.SADDeconnect();
#endif

  m_NbCarteChargee--;
  // On libère la DLL si elle a été chargée
   if(m_hLib!=NULL)
  {
    if (m_CartePresente)
    {
      // On vide la file d'emission hard
      ncdFlushTransmitQueue(m_gPortHandle,m_gChannelMask);

      // On arrête le thread
      if (m_ThreadID)
      {
        m_Arret=cVrai;
        WaitForSingleObject(m_hThread,5000);
        CloseHandle(m_hThread);
      }

      // On désactive le canal
      if (m_InitOk)   
      {
        ncdDeactivateChannel(m_gPortHandle,m_gChannelMask);
      }
      // On ferme le port Can
      if (m_gPortHandle != INVALID_PORTHANDLE)
      {
        ncdClosePort(m_gPortHandle);
        m_gPortHandle = INVALID_PORTHANDLE;
      }
      ncdCloseDriver();
    }

    if (m_NbCarteChargee == 0)    // On libère la librairie si plus de cartes actives
    {
       FreeLibrary(m_hLib);
      m_hLib=NULL;
    }
  }
}

//********************************************************************//
// Reinit can card needed for BUS OFF by example
//********************************************************************//
void tcCanVector::ReinitCard()
{
  m_CanCardMutex.Lock();
  //---- Deactivate channel previously loaded ----
  ncdDeactivateChannel(m_gPortHandle,m_gChannelMask);
  ncdClosePort(m_gPortHandle);
  m_gPortHandle = INVALID_PORTHANDLE;
  ncdCloseDriver();

  //---- Initialise channel -----
  int gHwType                 = m_ParamCan.Port>>8;           // HWTYPE
  int gHwChannel              = (m_ParamCan.Port&0xFF)%2;     // 2 ports par carte
  int gHwIndex                = (m_ParamCan.Port&0xFF)/2;     // N° de carte
  Vstatus Statut;
  VsetAcceptance acc;
  Vaccess gPermissionMask     = 0;
  m_gPortHandle               = INVALID_PORTHANDLE;

  Statut = ncdOpenDriver();
  if (Statut!=VSUCCESS)
  {
    char Buffer[256];
    sprintf(Buffer,"tcCanVector::tcCanVector> Can driver failed (%04X)",Statut);
    SetMsg(cErrorMsg,Buffer);
    return;
  }

  // On cherche le masque du canal Cancardx d'index 0 et de canal Port
  m_gChannelMask = ncdGetChannelMask(gHwType,gHwIndex,gHwChannel);
  if (m_gChannelMask==0) 
  {
    char Buffer[256];
    sprintf(Buffer,"tcCanVector::tcCanVector> %s not found",m_ParamCan.CanCardName);
    SetMsg(cErrorMsg,Buffer);
    return;
  }
  gPermissionMask = m_gChannelMask;
  // On ouvre le port
  Statut = ncdOpenPort(&m_gPortHandle,"JCAE",m_gChannelMask,m_gChannelMask,&gPermissionMask,m_ParamCan.QueueRead);
  if (Statut!=VSUCCESS)
  {
    char Buffer[256];
    sprintf(Buffer,"tcCanVector::tcCanVector> Can port failed (%04X)",Statut);
    SetMsg(cErrorMsg,Buffer);
    return;
  }
  if (gPermissionMask!=m_gChannelMask)
  {
    SetMsg(cErrorMsg,"tcCanVector::tcCanVector> Can card configuration unauthorized");
    return;
  }
  // On configure la vitesse
  Statut = ncdSetChannelBitrate(m_gPortHandle,m_gChannelMask,m_ParamCan.BaudRate);
  if (Statut!=VSUCCESS)
  {
    char Buffer[256];
    sprintf(Buffer,"tcCanVector::tcCanVector> Baudrate configuration failed (%04X)",Statut);
    SetMsg(cErrorMsg,Buffer);
    return;
  }
  // Change Ack
  Statut = ncdSetChannelOutput(m_gPortHandle,m_gChannelMask,(m_ParamCan.Spy==1)?OUTPUT_MODE_SILENT:OUTPUT_MODE_NORMAL);
  if (Statut!=VSUCCESS)
  {
    char Buffer[256];
    sprintf(Buffer,"tcCanVector::tcCanVector> spy configuration failed (%04X)",Statut);
    SetMsg(cErrorMsg,Buffer);
    return;
  }
  // Inhiber les notifications TX et TXRQ
  Statut = ncdSetChannelMode(m_gPortHandle,m_gChannelMask,0,0);
  if (Statut!=VSUCCESS)
  {
    char Buffer[256];
    sprintf(Buffer,"tcCanVector::tcCanVector> Tx and TXRQ disabling failed (%04X)",Statut);
    SetMsg(cErrorMsg,Buffer);
    return;
  }
  // On configure les masques d'acceptances
  acc.mask = m_ParamCan.MaskStd; // relevant=1
  acc.code = m_ParamCan.CodeStd;
  Statut = ncdSetChannelAcceptance(m_gPortHandle,m_gChannelMask,&acc);
  if (Statut!=VSUCCESS)
  {
    char Buffer[256];
    sprintf(Buffer,"tcCanVector::tcCanVector> STD Mask id configuration failed (%04X)",Statut);
    SetMsg(cErrorMsg,Buffer);
    return;
  }
  acc.mask = 0x80000000 | m_ParamCan.MaskXtd; // relevant=1
  acc.code = 0x80000000 | m_ParamCan.CodeXtd;
  Statut = ncdSetChannelAcceptance(m_gPortHandle,m_gChannelMask,&acc);
  if (Statut!=VSUCCESS)
  {
    char Buffer[256];
    sprintf(Buffer,"tcCanVector::tcCanVector> XTD Mask id configuration failed (%04X)",Statut);
    SetMsg(cErrorMsg,Buffer);
    return;
  }
  // On vide la queue de réception
  Statut = ncdFlushReceiveQueue(m_gPortHandle);
  if (Statut!=VSUCCESS)
  {
    char Buffer[256];
    sprintf(Buffer,"tcCanVector::tcCanVector> Reception queue erasing failed (%04X)",Statut);
    SetMsg(cErrorMsg,Buffer);
    return;
  }
  // On vide la queue de transmission
  Statut = ncdFlushTransmitQueue(m_gPortHandle,m_gChannelMask);
  if (Statut!=VSUCCESS)
  {
    char Buffer[256];
    sprintf(Buffer,"tcCanVector::tcCanVector> Emission queue erasing failed (%04X)",Statut);
    SetMsg(cErrorMsg,Buffer);
    return;
  }
  // On crée l'évènement pour recevoir
  m_gEventHandle = CreateEvent(NULL, FALSE, FALSE, NULL);
  Statut = ncdSetNotification(m_gPortHandle,(unsigned long*)&m_gEventHandle, 1);
  if (Statut!=VSUCCESS)
  {
    char Buffer[256];
    sprintf(Buffer,"tcCanVector::tcCanVector> Reception event configuration failed (%04X)",Statut);
    SetMsg(cErrorMsg,Buffer);
    return;
  }
  // On active le canal
  Statut = ncdActivateChannel(m_gPortHandle,m_gChannelMask);
  if (Statut!=VSUCCESS)
  {
    char Buffer[256];
    sprintf(Buffer,"tcCanVector::tcCanVector> Channel activation failed (%04X)",Statut);
    SetMsg(cErrorMsg,Buffer);
    return;
  }
  m_StatutReq = VSUCCESS;

  m_CanCardMutex.Unlock();
}

//********************************************************************//
// Can control
//********************************************************************//
U8 tcCanVector::Control(tCanControl CanControl,void* Param)
{
  Vstatus Statut;
  U8 ReturnValue = cFalse;
  if (CanControl == cCanChangeBaudrateControl)
  {
    m_CanCardMutex.Lock();
    // Deactivate channel
    Statut = ncdDeactivateChannel(m_gPortHandle,m_gChannelMask);
    if (Statut!=VSUCCESS)
    {
      char Buffer[256];
      sprintf(Buffer,"tcCanVector::tcCanVector> Channel deactivation failed (%04X)",Statut);
      SetMsg(cErrorMsg,Buffer);
      return cFalse;
    }
    // Change Baudrate
    Statut = ncdSetChannelBitrate(m_gPortHandle,m_gChannelMask,(U32)Param);
    if (Statut!=VSUCCESS)
    {
      char Buffer[256];
      sprintf(Buffer,"tcCanVector::tcCanVector> Baudrate configuration failed (%04X)",Statut);
      SetMsg(cErrorMsg,Buffer);
      return cFalse;
    }
    // Activate channel
    Statut = ncdActivateChannel(m_gPortHandle,m_gChannelMask);
    if (Statut!=VSUCCESS)
    {
      char Buffer[256];
      sprintf(Buffer,"tcCanVector::tcCanVector> Channel activation failed (%04X)",Statut);
      SetMsg(cErrorMsg,Buffer);
      return cFalse;
    }
    m_CanCardMutex.Unlock();
#ifndef DISABLE_SAD
    CString str;
    str.Format("Change Baudrate to %d Bds",(U32)Param);
    m_SADAdrFrame.SADAdrFrameText(str);
#endif  
    ReturnValue = cTrue;
  }
  else if (CanControl == cCanAckControl)
  {
    m_CanCardMutex.Lock();
    // Deactivate channel
    Statut = ncdDeactivateChannel(m_gPortHandle,m_gChannelMask);
    if (Statut!=VSUCCESS)
    {
      char Buffer[256];
      sprintf(Buffer,"tcCanVector::tcCanVector> Channel deactivation failed (%04X)",Statut);
      SetMsg(cErrorMsg,Buffer);
      return cFalse;
    }
    // Change Ack
    Statut = ncdSetChannelOutput(m_gPortHandle,m_gChannelMask,(((U32)Param==0)?OUTPUT_MODE_SILENT:OUTPUT_MODE_NORMAL));
    if (Statut!=VSUCCESS)
    {
      char Buffer[256];
      sprintf(Buffer,"tcCanVector::tcCanVector> Baudrate configuration failed (%04X)",Statut);
      SetMsg(cErrorMsg,Buffer);
      return cFalse;
    }
    // Activate channel
    Statut = ncdActivateChannel(m_gPortHandle,m_gChannelMask);
    if (Statut!=VSUCCESS)
    {
      char Buffer[256];
      sprintf(Buffer,"tcCanVector::tcCanVector> Channel activation failed (%04X)",Statut);
      SetMsg(cErrorMsg,Buffer);
      return cFalse;
    }
    m_CanCardMutex.Unlock();
    ReturnValue = cTrue;
  }

  return ReturnValue;
}

//********************************************************************//
// Transmission d'une trame
//********************************************************************//
U8 tcCanVector::ReqTrameUU(tObjetCanUU* pObjetCan,tTrameCan* pTrameCan)
{
  Vstatus StatutOld;
  Vevent vEvent;

  vEvent.tag                 = V_TRANSMIT_MSG;
  vEvent.tagData.msg.id      = pObjetCan->Id & ~cCanRemoteFrameFlag;
  vEvent.tagData.msg.flags   = pObjetCan->Id&cCanRemoteFrameFlag?VCAN_MSG_FLAG_REMOTE_FRAME:0;
  vEvent.tagData.msg.dlc     = pTrameCan->Longueur;
  if ( pObjetCan->ModeHighVoltage == cTrue )
  {
      // On vide la file d'emission hard
      ncdFlushTransmitQueue(m_gPortHandle,m_gChannelMask);

      // On positionne le flag wake-up
      vEvent.tagData.msg.flags = MSGFLAG_WAKEUP | MSGFLAG_OVERRUN;
  }
  memcpy(vEvent.tagData.msg.data,pTrameCan->Buffer,pTrameCan->Longueur);

  StatutOld=m_StatutReq;
  m_CanCardMutex.Lock();
  m_StatutReq = ncdTransmit(m_gPortHandle, m_gChannelMask, &vEvent);

  if (m_StatutReq == VERR_QUEUE_IS_FULL)    // Queue pleine
  {
    m_StatutReq = 0;    // Evite d'afficher un CanOk quand on peut à nouveau emettre
    return cFaux;
  }
  else if (m_StatutReq != 0)        // Erreur
  {
    if (m_StatutReq != StatutOld)
    {
      char Buffer[256];
      sprintf(Buffer,"tcCanVector::ReqTrameUU> Emission failed (%04X)",m_StatutReq);
      SetMsg(cErrorMsg,Buffer);
    }
    return cFaux;
  }
  else if (m_StatutReq != StatutOld)          // Ok
  {
    SetMsg(cErrorMsg,"tcCanVector::ReqTrameUU> Bus CAN OK");
  }
#ifndef DISABLE_SAD
  m_SADAdrFrame.SADAdrFrameTraceTx(pObjetCan->Id,pTrameCan->Longueur,pTrameCan->Buffer);
#endif  
  if (pObjetCan->FCallBackCon!=NULL)      // On signale une fin d"émission par callback si demandée
  {
    pObjetCan->FCallBackCon(pObjetCan,pObjetCan->ParamCallBackCon);
  }
  m_CanCardMutex.Unlock();
  return cVrai;
}

/*
void ErrorFrameTimeoutProc(void* Param)
{
  tcCanVector *pCanVector = (tcCanVector *)Param;
  if (pCanVector != NULL)
  {
    TRACE0 ("ErrorFrameTimeoutProc executed\n");
    pCanVector->ReinitCard();
    pCanVector->m_pTimerCan->EnleverTempoPeriodique (pCanVector->m_iErrorFrameTimeoutTimerID);
    pCanVector->m_iErrorFrameTimeoutTimerID = cNumeroTempoInvalide;
  }
}
*/

//********************************************************************//
// Thread Treatment
//********************************************************************//
void tcCanVector::ThreadTreatment()
{
  Vstatus       Statut;
  Vevent        *pEvent;
  U32       Id;
  BOOL bErrFrame;

  while(m_Arret == cFaux)
  {
    // On attend les évènement en réception
    if (WaitForSingleObject(m_gEventHandle,500) == WAIT_OBJECT_0)
    {
      do
      {
        Statut = ncdReceive1(m_gPortHandle,&pEvent);
        if (Statut == VSUCCESS)
        {
          //--- On a reçu un message ---
          if (pEvent->tag == V_RECEIVE_MSG) 
          {
            Id = pEvent->tagData.msg.id & 0x1FFFFFFF;

            if (pEvent->tagData.msg.flags & VCAN_MSG_FLAG_REMOTE_FRAME)
            {
              Id |= cCanRemoteFrameFlag;
              pEvent->tagData.msg.flags &=~ VCAN_MSG_FLAG_REMOTE_FRAME;
            }
            if (pEvent->tagData.msg.flags & VCAN_MSG_FLAG_NERR)
            {
#ifndef DISABLE_SAD
              m_SADAdrFrame.SADAdrFrameTextRx(cCanErrorNerr,"Error Nerr");
#endif
              pEvent->tagData.msg.flags &=~ VCAN_MSG_FLAG_NERR;
            }

			bErrFrame = FALSE;
            m_CanCardMutex.Lock();
            if (pEvent->tagData.msg.flags == 0 || pEvent->tagData.msg.flags == VCAN_MSG_FLAG_WAKEUP)
            {
/*            if (m_iErrorFrameTimeoutTimerID != cNumeroTempoInvalide)
              {
                m_pTimerCan->EnleverTempoPeriodique (m_iErrorFrameTimeoutTimerID);
                m_iErrorFrameTimeoutTimerID = cNumeroTempoInvalide;
              }
*/
#ifndef DISABLE_SAD // XTD frame already begins by 0x80000000
              m_SADAdrFrame.SADAdrFrameTraceRx(Id,pEvent->tagData.msg.dlc,pEvent->tagData.msg.data);
#endif
              NotifyRx(Id,pEvent->tagData.msg.dlc,pEvent->tagData.msg.data);
            }
            else if (pEvent->tagData.msg.flags & VCAN_MSG_FLAG_ERROR_FRAME)   // Error frame
            {
#ifndef DISABLE_SAD
              m_SADAdrFrame.SADAdrFrameTextRx(cCanErrorFrameError,"Error frame");
#endif
              NotifyRx(cCanErrorFrameError,0,NULL);
			  bErrFrame = TRUE;
/*
              if (m_iErrorFrameTimeoutTimerID == cNumeroTempoInvalide)
              {
                m_iErrorFrameTimeoutTimerID = m_pTimerCan->InsererTempoPeriodique (500, ErrorFrameTimeoutProc, this);
              }
*/
            }
            m_CanCardMutex.Unlock();
			if (bErrFrame)
			{
				ReinitCard();
			}
          }
          else if (pEvent->tag == V_CHIP_STATE)
          {
            if (pEvent->tagData.chipState.busStatus == CHIPSTAT_BUSOFF)
            {

#ifndef DISABLE_SAD
              m_SADAdrFrame.SADAdrFrameTextRx(cCanErrorBusOff,"Error BUS_OFF, Reinit...");
#endif
              m_CanCardMutex.Lock();
              NotifyRx(cCanErrorBusOff,0,NULL);
              m_CanCardMutex.Unlock();
              ReinitCard();
            }
//            else if (pEvent->tagData.chipState.busStatus == CHIPSTAT_ERROR_PASSIVE)
//            {
//#ifndef DISABLE_SAD
//              m_SADAdrFrame.SADAdrFrameTextRx(cCanErrorBusOff,"Error PASSIVE");
//#endif
//            }
//            else if (pEvent->tagData.chipState.busStatus == CHIPSTAT_ERROR_ACTIVE)
//            {
//#ifndef DISABLE_SAD
//              m_SADAdrFrame.SADAdrFrameTextRx(cCanErrorBusOff,"Error ACTIVE");
//#endif
//            }

//#ifndef DISABLE_SAD
//            m_SADAdrFrame.SADAdrFrameTextRx(cCanErrorFrameFlag,ncdGetEventString(pEvent));
//#endif
          }
        }
      }while(!Statut);
    }
  }
}

//********************************************************************//
// Thread gérant les réceptions de trames
//********************************************************************//
DWORD WINAPI tcCanVector::CorpsThread( LPVOID Param )
{
  SetThreadPriority( GetCurrentThread(), THREAD_PRIORITY_ABOVE_NORMAL);
  ((tcCanVector*)Param)->ThreadTreatment();
  SetThreadPriority( GetCurrentThread(), THREAD_PRIORITY_NORMAL);
  ((tcCanVector*)Param)->m_ThreadID = 0;
  return cVrai;
}

#endif    // Le canal can est utilisé