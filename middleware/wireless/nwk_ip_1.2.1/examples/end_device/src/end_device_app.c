/*
 * Copyright (c) 2014 - 2015, Freescale Semiconductor, Inc.
 * Copyright 2016-2017 NXP
 *
 * Redistribution and use in source and binary forms, with or without modification,
 * are permitted provided that the following conditions are met:
 *
 * o Redistributions of source code must retain the above copyright notice, this list
 *   of conditions and the following disclaimer.
 *
 * o Redistributions in binary form must reproduce the above copyright notice, this
 *   list of conditions and the following disclaimer in the documentation and/or
 *   other materials provided with the distribution.
 *
 * o Neither the name of the copyright holder nor the names of its
 *   contributors may be used to endorse or promote products derived from this
 *   software without specific prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS" AND
 * ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED
 * WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE
 * DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT HOLDER OR CONTRIBUTORS BE LIABLE FOR
 * ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES
 * (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES;
 * LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON
 * ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
 * (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS
 * SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 */

/*!=================================================================================================
\file       end_device_app.c
\brief      This is a public source file for the end device demo application.
==================================================================================================*/

/*==================================================================================================
Include Files
==================================================================================================*/
/* General Includes */
#include "EmbeddedTypes.h"
#include <string.h>

/* FSL Framework */
#include "shell.h"
#include "Keyboard.h"
#include "RNG_Interface.h"
#include "PWR_Interface.h"

/* Network */
#include "ip_if_management.h"
#include "event_manager.h"

/* Application */
#include "end_device_app.h"
#include "shell_ip.h"
#include "thread_utils.h"
#include "thread_network.h"
#include "thread_app_callbacks.h"
#include "app_init.h"
#include "app_stack_config.h"
#include "app_thread_config.h"
#include "app_led.h"
#include "app_temp_sensor.h"
#include "coap.h"
#include "app_socket_utils.h"
#if THR_ENABLE_EVENT_MONITORING
#include "app_event_monitoring.h"
#endif
#if THR_ENABLE_MGMT_DIAGNOSTICS
#include "thread_mgmt.h"
#include "thci.h"
#endif
#if UDP_ECHO_PROTOCOL
#include "app_echo_udp.h"
#endif

/*@Lab - Accelerometer app & pin_mux headers*/
#include "app_accel_sensor.h"
#include "pin_mux.h"

/*==================================================================================================
Private macros
==================================================================================================*/
#ifndef APP_MSG_QUEUE_SIZE
    #define APP_MSG_QUEUE_SIZE                  20
#endif

#if (THREAD_USE_SHELL == FALSE)
    #define shell_write(a)
    #define shell_refresh()
    #define shell_printf(a,...)
#endif

#define gThrDefaultInstanceId_c                 0

#define APP_LED_URI_PATH                        "/led"
#define APP_TEMP_URI_PATH                       "/temp"
#define APP_SINK_URI_PATH                       "/sink"

#define APP_DEFAULT_DEST_ADDR                   in6addr_realmlocal_allthreadnodes

#define APP_SW_WAKE_UP_TIMEOUT                  5000 /* miliseconds */

/*@Lab - URI definition to get data from the accelerometer*/
#define APP_ACCEL_URI_PATH "/accel"
#define APP_EQUIPO4_URI_PATH "/equipo4"
/*@Lab - Define Accelerometer event*/
#define gAppAccNoEvent_c 0xFF

/*==================================================================================================
Private type definitions
==================================================================================================*/

/*==================================================================================================
Private global variables declarations
==================================================================================================*/
static instanceId_t mThrInstanceId = gInvalidInstanceId_c;    /*!< Thread Instance ID */

static bool_t mJoiningIsAppInitiated = FALSE;

/*@Lab - Application Timer*/
static tmrTimerID_t mLEDOffTimerID = gTmrInvalidTimerID_c;
static tmrTimerID_t mEquipo4GETTimerID = gTmrInvalidTimerID_c;
/*@Lab - Private global variables*/
static uint8_t mAccLastEvent = gAppAccNoEvent_c;

/*==================================================================================================
Private prototypes
==================================================================================================*/
static void App_HandleKeyboard(void *param);
static void App_UpdateStateLeds(appDeviceState_t deviceState);
static void APP_JoinEventsHandler(thrEvCode_t evCode);

static void APP_SwWakeUpCb(void *pParam);
static void APP_InitCoapDemo(void);
static void APP_ReportTemp(void *pParam);
static void APP_SendDataSinkCreate(void *pParam);
static void APP_SendDataSinkRelease(void *pParam);
#if gKBD_KeysCount_c > 1
static void APP_SendLedRgbOn(void *pParam);
static void APP_SendLedRgbOff(void *pParam);
static void APP_SendLedFlash(void *pParam);
static void APP_SendLedColorWheel(void *pParam);
#endif
static void APP_LocalDataSinkRelease(void *pParam);
static void APP_ProcessLedCmd(uint8_t *pCommand, uint8_t dataLen);
static void APP_CoapGenericCallback(coapSessionStatus_t sessionStatus, void *pData, coapSession_t *pSession, uint32_t dataLen);
static void APP_CoapLedCb(coapSessionStatus_t sessionStatus, void *pData, coapSession_t *pSession, uint32_t dataLen);
static void APP_CoapTempCb(coapSessionStatus_t sessionStatus, void *pData, coapSession_t *pSession, uint32_t dataLen);
static void APP_CoapSinkCb(coapSessionStatus_t sessionStatus, void *pData, coapSession_t *pSession, uint32_t dataLen);

/*@Lab - Private prototypes*/
static void Accel_Callback(uint8_t events);
static void APP_ReportAccel(void *pParam);
static void timerTurnOffLEDsCB(void *param);
static void APP_CoapAccelCb(coapSessionStatus_t sessionStatus,void *pData,coapSession_t *pSession,uint32_t dataLen);
static void APP_CoapEquipo4Cb(coapSessionStatus_t sessionStatus,void *pData,coapSession_t *pSession,uint32_t dataLen);
static void APP_AccCallback(void);

static void APP_CoapEquipo4Cb(coapSessionStatus_t sessionStatus,void *pData,coapSession_t *pSession,uint32_t dataLen);
static void APP_TimmerEquipo4Cb(void *pParam);

/*==================================================================================================
Public global variables declarations
==================================================================================================*/
const coapUriPath_t gAPP_LED_URI_PATH  = {SizeOfString(APP_LED_URI_PATH), (uint8_t *)APP_LED_URI_PATH};
const coapUriPath_t gAPP_TEMP_URI_PATH = {SizeOfString(APP_TEMP_URI_PATH), (uint8_t *)APP_TEMP_URI_PATH};
const coapUriPath_t gAPP_SINK_URI_PATH = {SizeOfString(APP_SINK_URI_PATH), (uint8_t *)APP_SINK_URI_PATH};


/* Application state/mode */
appDeviceState_t gAppDeviceState[THR_MAX_INSTANCES];
appDeviceMode_t gAppDeviceMode[THR_MAX_INSTANCES];

/* Pointer application task message queue */
taskMsgQueue_t *mpAppThreadMsgQueue;

/* Flag used to stop the attaching retries */
bool_t gbRetryInterrupt = TRUE;

/* CoAP instance */
uint8_t mAppCoapInstId = THR_ALL_FFs8;
/* Destination address for CoAP commands */
ipAddr_t gCoapDestAddress;
/* Timer used to keep the device in Up state for an amount of time */
tmrTimerID_t gAppSwWakeUpTimer = gTmrInvalidTimerID_c;

extern bool_t gEnable802154TxLed;

/*@Lab - Declare URI path for Accelerometer data*/
const coapUriPath_t gAPP_ACCEL_URI_PATH = {SizeOfString(APP_ACCEL_URI_PATH), (uint8_t *)APP_ACCEL_URI_PATH};
const coapUriPath_t gAPP_EQUIPO4_URI_PATH = {SizeOfString(APP_EQUIPO4_URI_PATH), (uint8_t *)APP_EQUIPO4_URI_PATH};

/*==================================================================================================
Public functions
==================================================================================================*/
/*!*************************************************************************************************
\fn     void APP_Init(void)
\brief  This function is used to initialize application.

***************************************************************************************************/
void APP_Init
(
    void
)
{
    /* Initialize pointer to application task message queue */
    mpAppThreadMsgQueue = &appThreadMsgQueue;

    /* Initialize main thread message queue */
    ListInit(&appThreadMsgQueue.msgQueue,APP_MSG_QUEUE_SIZE);

    /* Set default device mode/state */
    APP_SetState(gThrDefaultInstanceId_c, gDeviceState_FactoryDefault_c);
    APP_SetMode(gThrDefaultInstanceId_c, gDeviceMode_Configuration_c);

    /* Initialize keyboard handler */
    pfAppKeyboardHandler = App_HandleKeyboard;

    /* Use one instance ID for application */
    mThrInstanceId = gThrDefaultInstanceId_c;


#if THR_ENABLE_EVENT_MONITORING
    /* Initialize event monitoring */
    APP_InitEventMonitor(mThrInstanceId);
#endif

    if(gThrStatus_Success_c == THR_StartInstance(mThrInstanceId, pStackCfg[0]))
    {
        /* Initialize CoAP demo */
        APP_InitCoapDemo();


#if USE_TEMPERATURE_SENSOR
        /* Initialize Temperature sensor/ADC module*/
        APP_InitADC(ADC_0);
#endif

        /*@Lab - Initialize Accelerometer*/
#if USE_ACCELEROMETER
        /*Init I2C module */
        BOARD_InitI2C();
        if (APP_InitAccelerometer(Accel_Callback) != kStatus_Success)
        {
        	shell_printf("\r\nAccelerometer initialization failed\r\n");
        }
        else
        {
        	shell_printf("\r\nAccelerometer OK\r\n");
        }
#endif
        /*@Lab - Allocate timer for the LED off callback */
        if(mLEDOffTimerID == gTmrInvalidTimerID_c)
        {
        	mLEDOffTimerID = TMR_AllocateTimer();
        	mEquipo4GETTimerID = TMR_AllocateTimer();

        }

#if THREAD_USE_THCI && THR_ENABLE_MGMT_DIAGNOSTICS
        (void)MgmtDiagnostic_RegisterAppCb(THCI_MgmtDiagnosticAppCb);
#endif

#if THREAD_USE_SHELL && SOCK_DEMO
        /* Initialize use sockets - used from shell */
        APP_InitUserSockets(mpAppThreadMsgQueue);
#endif

#if APP_AUTOSTART
       if(!THR_GetAttr_IsDevConnected(mThrInstanceId))
       {
          mJoiningIsAppInitiated = TRUE;

          if(THR_NwkJoin(mThrInstanceId, THR_APP_JOIN_DISCOVERY_METHOD) != gThrStatus_Success_c)
          {
              /* User can treat join failure according to their application */
          }
       }
#endif
    }
}

/*!*************************************************************************************************
\fn     void App_Handler(void)
\brief  Application Handler. In this configuration is called on the task with the lowest priority
***************************************************************************************************/
void APP_Handler
(
    void
)
{
    bool_t handleMsg = TRUE;

    while(handleMsg == TRUE)
    {
        handleMsg = NWKU_MsgHandler(&appThreadMsgQueue);
        /* For BareMetal break the while(1) after 1 run */
        if(!gUseRtos_c && MSG_Pending(&appThreadMsgQueue.msgQueue))
        {
            (void)OSA_EventSet(appThreadMsgQueue.taskEventId, NWKU_GENERIC_MSG_EVENT);
            break;
        }
    }
}

/*!*************************************************************************************************
\fn     void APP_NwkScanHandler(void *param)
\brief  This function is used to handle network scan results in asynchronous mode.
***************************************************************************************************/
void APP_NwkScanHandler
(
    void *param
)
{
    thrEvmParams_t *pEventParams = (thrEvmParams_t *)param;
    thrNwkScanResults_t *pScanResults = &pEventParams->pEventData->nwkScanCnf;

    /* Handle the network scan result here */
    if(pScanResults)
    {
#if THREAD_USE_SHELL
        SHELL_NwkScanPrint(pScanResults);
#endif
        MEM_BufferFree(pScanResults);
    }
    /* Free Event Buffer */
    MEM_BufferFree(pEventParams);
}

/*!*************************************************************************************************
\fn     void Stack_to_APP_Handler(void *param)
\brief  This function is used to handle stack events in asynchronous mode.

\param  [in]    param    Pointer to stack event
***************************************************************************************************/
void Stack_to_APP_Handler
(
    void *param
)
{
    thrEvmParams_t *pEventParams = (thrEvmParams_t *)param;

    switch(pEventParams->code)
    {
        case gThrEv_GeneralInd_ResetToFactoryDefault_c:
            App_UpdateStateLeds(gDeviceState_FactoryDefault_c);
            break;

        case gThrEv_GeneralInd_InstanceRestoreStarted_c:
        case gThrEv_GeneralInd_ConnectingStarted_c:
            APP_SetMode(mThrInstanceId, gDeviceMode_Configuration_c);
            App_UpdateStateLeds(gDeviceState_JoiningOrAttaching_c);
            gEnable802154TxLed = FALSE;
            break;

        case gThrEv_NwkJoinCnf_Success_c:
        case gThrEv_NwkJoinCnf_Failed_c:
            APP_JoinEventsHandler(pEventParams->code);
            break;

        case gThrEv_GeneralInd_Connected_c:
            App_UpdateStateLeds(gDeviceState_NwkConnected_c);
            /* Set application CoAP destination to all nodes on connected network */
            gCoapDestAddress = APP_DEFAULT_DEST_ADDR;
            APP_SetMode(mThrInstanceId, gDeviceMode_Application_c);
            /* Enable LED for 80215.4 tx activity */
            gEnable802154TxLed = TRUE;
#if UDP_ECHO_PROTOCOL
            ECHO_ProtocolInit(mpAppThreadMsgQueue);
#endif
            /*@Lab - Enable Accelerometer reading once in the network*/
            APP_EnableAccelerometer();
            gEnable802154TxLed = FALSE; //Disable default TX activity LED

            /* Allocate Equipo4 Timmer */


            break;

        case gThrEv_GeneralInd_ConnectingFailed_c:
        case gThrEv_GeneralInd_Disconnected_c:
            APP_SetMode(mThrInstanceId, gDeviceMode_Configuration_c);
            App_UpdateStateLeds(gDeviceState_NwkFailure_c);

            TMR_StopTimer(mEquipo4GETTimerID);

            break;

#if gLpmIncluded_d
        case gThrEv_GeneralInd_AllowDeviceToSleep_c:
            PWR_AllowDeviceToSleep();
            break;

        case gThrEv_GeneralInd_DisallowDeviceToSleep_c:
            PWR_DisallowDeviceToSleep();
            break;
#endif
        default:
            break;
    }

    /* Free event buffer */
    MEM_BufferFree(pEventParams->pEventData);
    MEM_BufferFree(pEventParams);
}

/*!*************************************************************************************************
\fn     void APP_Commissioning_Handler(void *param)
\brief  This function is used to handle Commissioning events in synchronous mode.

\param  [in]    param    Pointer to Commissioning event
***************************************************************************************************/
void APP_Commissioning_Handler
(
    void *param
)
{
    thrEvmParams_t *pEventParams = (thrEvmParams_t *)param;

    switch(pEventParams->code)
    {
        /* Joiner Events */
        case gThrEv_MeshCop_JoinerDiscoveryStarted_c:
            break;
        case gThrEv_MeshCop_JoinerDiscoveryFailed_c:
            break;
        case gThrEv_MeshCop_JoinerDiscoveryFailedFiltered_c:
            break;
        case gThrEv_MeshCop_JoinerDiscoverySuccess_c:
            break;
        case gThrEv_MeshCop_JoinerDtlsSessionStarted_c:
            App_UpdateStateLeds(gDeviceState_JoiningOrAttaching_c);
            break;
        case gThrEv_MeshCop_JoinerDtlsError_c:
        case gThrEv_MeshCop_JoinerError_c:
            App_UpdateStateLeds(gDeviceState_FactoryDefault_c);
            break;
        case gThrEv_MeshCop_JoinerAccepted_c:
            break;
    }

    /* Free event buffer */
    MEM_BufferFree(pEventParams);
}

/*!*************************************************************************************************
\fn     void App_SedWakeUpFromKeyBoard(void)
\brief  This is a callback fuction called when the device is waked from keyboard.
***************************************************************************************************/
void App_SedWakeUpFromKeyBoard
(
    void
)
{
    /* Keep the device Up for 5 seconds - using a timer */
    if(gAppSwWakeUpTimer == gTmrInvalidTimerID_c) 
    {
        gAppSwWakeUpTimer = TMR_AllocateTimer();
    }

    if(gAppSwWakeUpTimer != gTmrInvalidTimerID_c)
    {
        TMR_StartTimer(gAppSwWakeUpTimer, gTmrSingleShotTimer_c, APP_SW_WAKE_UP_TIMEOUT, APP_SwWakeUpCb, NULL);

    }
}
/*==================================================================================================
Private functions
==================================================================================================*/
/*!*************************************************************************************************
\private
\fn     static void APP_InitCoapDemo(void)
\brief  Initialize CoAP demo.
***************************************************************************************************/
static void APP_InitCoapDemo
(
    void
)
{
    coapRegCbParams_t cbParams[] =  {{APP_CoapLedCb,  (coapUriPath_t *)&gAPP_LED_URI_PATH},
                                     {APP_CoapTempCb, (coapUriPath_t *)&gAPP_TEMP_URI_PATH},
                                     {APP_CoapSinkCb, (coapUriPath_t *)&gAPP_SINK_URI_PATH},
    								 {APP_CoapAccelCb, (coapUriPath_t*)&gAPP_ACCEL_URI_PATH},
									 {APP_CoapEquipo4Cb, (coapUriPath_t *)&gAPP_EQUIPO4_URI_PATH},}; //@Lab - Register Accelerometer CoAP service
    /* Register Services in COAP */
    coapStartUnsecParams_t coapParams = {COAP_DEFAULT_PORT, AF_INET6};
    mAppCoapInstId = COAP_CreateInstance(NULL, &coapParams, gIpIfSlp0_c, (coapRegCbParams_t *) cbParams,
                                         NumberOfElements(cbParams));
}

/*!*************************************************************************************************
\private
\fn     static void APP_ConfigModeHandleKeyboard(uint32_t keyEvent)
\brief  This is a handler for KBD module events. Device is in configuration mode.

\param  [in]    keyEvent    The keyboard module event
***************************************************************************************************/
static void APP_ConfigModeHandleKeyboard
(
    uint32_t keyEvent
)
{
    switch(keyEvent)
    {
        case gKBD_EventPB1_c:
#if gKBD_KeysCount_c > 1
        case gKBD_EventPB2_c:
        case gKBD_EventPB3_c:
        case gKBD_EventPB4_c:
#endif          
            if((APP_GetState(mThrInstanceId) == gDeviceState_FactoryDefault_c) ||
               (APP_GetState(mThrInstanceId) == gDeviceState_NwkFailure_c))
            {
                App_UpdateStateLeds(gDeviceState_JoiningOrAttaching_c);

                mJoiningIsAppInitiated = TRUE;

                /* join the network */
                if(THR_NwkJoin(mThrInstanceId, THR_APP_JOIN_DISCOVERY_METHOD) != gThrStatus_Success_c)
                {
                    /* User can treat join failure according to their application */
                }


            }
            break;
        case gKBD_EventLongPB1_c:
#if gKBD_KeysCount_c > 1
        case gKBD_EventLongPB2_c:
        case gKBD_EventLongPB3_c:
        case gKBD_EventLongPB4_c:
#endif
            break;
        /* Factory reset */
        case gKBD_EventVeryLongPB1_c:
#if gKBD_KeysCount_c > 1
        case gKBD_EventVeryLongPB2_c:
        case gKBD_EventVeryLongPB3_c:
        case gKBD_EventVeryLongPB4_c:
#endif
            THR_FactoryReset();
            break;
        default:
            break;
    }
}

/*!*************************************************************************************************
\private
\fn     static void APP_AppModeHandleKeyboard(uint32_t keyEvent)
\brief  This is a handler for KBD module events. Device is in application mode.

\param  [in]    keyEvent    The keyboard module event
***************************************************************************************************/
static void APP_AppModeHandleKeyboard
(
    uint32_t keyEvent
)
{
#if gLpmIncluded_d  
    /* restart wake up timer */
    App_SedWakeUpFromKeyBoard();
#endif 
  
    switch(keyEvent)
    {
        case gKBD_EventPB1_c:
            /* Data sink create */
            //(void)NWKU_SendMsg(APP_SendDataSinkCreate, NULL, mpAppThreadMsgQueue);
        	/*@Lab - Report Accelerometer value */
        	(void)NWKU_SendMsg(APP_ReportAccel, (void*)gAccel_All_c, mpAppThreadMsgQueue);
        	Led_UpdateRgbState(255,0,255);
        	App_UpdateStateLeds(gDeviceState_AppLedRgb_c);
        	TMR_StartSingleShotTimer(mLEDOffTimerID, 70, timerTurnOffLEDsCB, NULL);
        	TMR_StartSingleShotTimer(mEquipo4GETTimerID, 1000, APP_TimmerEquipo4Cb, NULL);
            break;
#if gKBD_KeysCount_c > 1
        case gKBD_EventPB2_c:
            /* Report temperature */
            (void)NWKU_SendMsg(APP_ReportTemp, NULL, mpAppThreadMsgQueue);
            break;
        case gKBD_EventPB3_c:
            /* Remote led RGB - on */
            (void)NWKU_SendMsg(APP_SendLedRgbOn, NULL, mpAppThreadMsgQueue);
            break;
        case gKBD_EventPB4_c:
            /* Remote led RGB - off */
            (void)NWKU_SendMsg(APP_SendLedRgbOff, NULL, mpAppThreadMsgQueue);
            break;
#endif
        case gKBD_EventLongPB1_c:
            /* Remote data sink release */
            (void)NWKU_SendMsg(APP_SendDataSinkRelease, NULL, mpAppThreadMsgQueue);
            break;
#if gKBD_KeysCount_c > 1
        case gKBD_EventLongPB2_c:
            /* Local data sink release */
            (void)NWKU_SendMsg(APP_LocalDataSinkRelease, NULL, mpAppThreadMsgQueue);
            break;
        case gKBD_EventLongPB3_c:
            /* Remote led flash */
            (void)NWKU_SendMsg(APP_SendLedFlash, NULL, mpAppThreadMsgQueue);
            break;
        case gKBD_EventLongPB4_c:
            /* Remote led - color wheel */
            (void)NWKU_SendMsg(APP_SendLedColorWheel, NULL, mpAppThreadMsgQueue);
            break;
#endif
        case gKBD_EventVeryLongPB1_c:
#if gKBD_KeysCount_c > 1
        case gKBD_EventVeryLongPB2_c: 
        case gKBD_EventVeryLongPB3_c: 
        case gKBD_EventVeryLongPB4_c:
#endif
            /* Factory reset */
            THR_FactoryReset();
            break;
        default:
            break;
    }
}

/*!*************************************************************************************************
\private
\fn     static void App_HandleKeyboard(void *param)
\brief  This is a handler for KBD module events.

\param  [in]    param    The keyboard module event
***************************************************************************************************/
static void App_HandleKeyboard
(
    void *param
)
{
    uint32_t events = (uint32_t)(param);

    if(APP_GetMode(mThrInstanceId) == gDeviceMode_Configuration_c)
    {
        /* Device is in configuration mode */
        APP_ConfigModeHandleKeyboard(events);
    }
    else
    {
        /* Device is in application mode */
        APP_AppModeHandleKeyboard(events);
    }
}

/*!*************************************************************************************************
\private
\fn     static void App_UpdateLedState(appDeviceState_t deviceState)
\brief  Called when Application state and LEDs must be updated.

\param  [in]    deviceState    The current device state
***************************************************************************************************/
static void App_UpdateStateLeds
(
    appDeviceState_t deviceState
)
{
    APP_SetState(mThrInstanceId, deviceState);
    Led_SetState(APP_GetMode(mThrInstanceId), APP_GetState(mThrInstanceId));
}

/*!*************************************************************************************************
\private
\fn     static void APP_JoinEventsHandler(thrEvCode_t evCode)
\brief  This function is used to the handle join failed event.

\param  [in]    evCode    Event code
***************************************************************************************************/
static void APP_JoinEventsHandler
(
    thrEvCode_t evCode
)
{
    if(mJoiningIsAppInitiated)
    {
        if(evCode == gThrEv_NwkJoinCnf_Failed_c)
        {
            if(gbRetryInterrupt)
            {
                /* Retry to join the network */
                if(THR_NwkJoin(mThrInstanceId, THR_APP_JOIN_DISCOVERY_METHOD) != gThrStatus_Success_c)
                {
                    /* User can treat join failure according to their application */
                }
                return;
            }
            mJoiningIsAppInitiated = FALSE;
        }
        else if(evCode == gThrEv_NwkJoinCnf_Success_c)
        {
            mJoiningIsAppInitiated = FALSE;
        }
    }
}

/*!*************************************************************************************************
\fn     static void APP_SwWakeUpCb(void *pParam)
\brief  This is a callback fuction called when the device is waked from keyboard. 
        After this callback the device will enter in low power

\param  [in]    pParam    Not used
***************************************************************************************************/
static void APP_SwWakeUpCb
(
    void *pParam
)
{
    TMR_StopTimer(gAppSwWakeUpTimer);
    PWR_AllowDeviceToSleep();
}

/*==================================================================================================
  Coap Demo functions:
==================================================================================================*/
/*!*************************************************************************************************
\private
\fn     static void APP_CoapGenericCallback(coapSessionStatus_t sessionStatus, void *pData,
                                            coapSession_t *pSession, uint32_t dataLen)
\brief  This function is the generic callback function for CoAP message.

\param  [in]    sessionStatus   Status for CoAP session
\param  [in]    pData           Pointer to CoAP message payload
\param  [in]    pSession        Pointer to CoAP session
\param  [in]    dataLen         Length of CoAP payload
***************************************************************************************************/
static void APP_CoapGenericCallback
(
    coapSessionStatus_t sessionStatus,
    void *pData,
    coapSession_t *pSession,
    uint32_t dataLen
)
{
    /* If no ACK was received, try again */
    if(sessionStatus == gCoapFailure_c)
    {
        if(FLib_MemCmp(pSession->pUriPath->pUriPath, (coapUriPath_t*)&gAPP_TEMP_URI_PATH.pUriPath,
                       pSession->pUriPath->length))
        {
            (void)NWKU_SendMsg(APP_ReportTemp, NULL, mpAppThreadMsgQueue);
        }
    }
    /* Process data, if any */
}

/*!*************************************************************************************************
\private
\fn     static void APP_ReportTemp(void *pParam)
\brief  This open a socket and report the temperature to gCoapDestAddress.

\param  [in]    pParam    Not used
***************************************************************************************************/
static void APP_ReportTemp
(
    void *pParam
)
{
    coapSession_t *pSession = NULL;
    /* Get Temperature */
    uint8_t *pTempString = App_GetTempDataString();
    uint32_t ackPloadSize;
    ifHandle_t ifHandle = THR_GetIpIfPtrByInstId(mThrInstanceId);

    if(!IP_IF_IsMyAddr(ifHandle->ifUniqueId, &gCoapDestAddress))
    {
        pSession = COAP_OpenSession(mAppCoapInstId);

        if(NULL != pSession)
        {
            coapMsgTypesAndCodes_t coapMessageType = gCoapMsgTypeNonPost_c;

            pSession->pCallback = NULL;
            FLib_MemCpy(&pSession->remoteAddr, &gCoapDestAddress, sizeof(ipAddr_t));
            ackPloadSize = strlen((char *)pTempString);
            COAP_SetUriPath(pSession, (coapUriPath_t *)&gAPP_TEMP_URI_PATH);

            if(!IP6_IsMulticastAddr(&gCoapDestAddress))
            {
                coapMessageType = gCoapMsgTypeConPost_c;
                pSession->pCallback = APP_CoapGenericCallback;
            }

            COAP_Send(pSession, coapMessageType, pTempString, ackPloadSize);
        }
    }
    /* Print temperature in shell */
    shell_write("\r");
    shell_write((char *)pTempString);
    shell_refresh();
    MEM_BufferFree(pTempString);
}

/*!*************************************************************************************************
\private
\fn     static nwkStatus_t APP_SendDataSinkCommand(uint8_t *pCommand, uint8_t dataLen)
\brief  This function is used to send a Data Sink command to APP_DEFAULT_DEST_ADDR.

\param  [in]    pCommand       Pointer to command data
\param  [in]    dataLen        Data length

\return         nwkStatus_t    Status of the command
***************************************************************************************************/
static nwkStatus_t APP_SendDataSinkCommand
(
    uint8_t *pCommand,
    uint8_t dataLen
)
{
    nwkStatus_t status = gNwkStatusFail_c;
    coapSession_t *pSession = COAP_OpenSession(mAppCoapInstId);

    if(pSession)
    {
        ipAddr_t coapDestAddress = APP_DEFAULT_DEST_ADDR;

        pSession->pCallback = NULL;
        FLib_MemCpy(&pSession->remoteAddr, &coapDestAddress, sizeof(ipAddr_t));
        COAP_SetUriPath(pSession, (coapUriPath_t*)&gAPP_SINK_URI_PATH);
        status = COAP_Send(pSession, gCoapMsgTypeNonPost_c, pCommand, dataLen);
    }

    return status;
}

/*!*************************************************************************************************
\private
\fn     static void APP_SendDataSinkCreate(void *pParam)
\brief  This function is used to send a Data Sink Create command to APP_DEFAULT_DEST_ADDR.

\param  [in]    pParam    Not used
***************************************************************************************************/
static void APP_SendDataSinkCreate
(
    void *pParam
)
{
    uint8_t aCommand[] = {"create"};

    /* Send command over the air */
    if(APP_SendDataSinkCommand(aCommand, sizeof(aCommand)) == gNwkStatusSuccess_c)
    {
        /* Local data sink create */
        (void)THR_GetIP6Addr(mThrInstanceId, gMLEIDAddr_c, &gCoapDestAddress, NULL);
    }
}

/*!*************************************************************************************************
\private
\fn     static void APP_SendDataSinkRelease(void *pParam)
\brief  This function is used to send a Data Sink Release command to APP_DEFAULT_DEST_ADDR.

\param  [in]    pParam    Pointer to stack event
***************************************************************************************************/
static void APP_SendDataSinkRelease
(
    void *pParam
)
{

    uint8_t aCommand[] = {"release"};

    /* Send command over the air */
    if(APP_SendDataSinkCommand(aCommand, sizeof(aCommand)) == gNwkStatusSuccess_c)
    {
        /* Local data sink release */
        APP_LocalDataSinkRelease(pParam);
    }
}

#if gKBD_KeysCount_c > 1
/*!*************************************************************************************************
\private
\fn     static void APP_SendLedCommand(uint8_t *pCommand, uint8_t dataLen)
\brief  This function is used to send a Led command to gCoapDestAddress.

\param  [in]    pCommand    Pointer to command data
\param  [in]    dataLen     Data length
***************************************************************************************************/
static void APP_SendLedCommand
(
    uint8_t *pCommand,
    uint8_t dataLen
)
{
    ifHandle_t ifHandle = THR_GetIpIfPtrByInstId(mThrInstanceId);

    if(!IP_IF_IsMyAddr(ifHandle->ifUniqueId, &gCoapDestAddress))
    {
        coapSession_t *pSession = COAP_OpenSession(mAppCoapInstId);

        if(pSession)
        {
            coapMsgTypesAndCodes_t coapMessageType = gCoapMsgTypeNonPost_c;

            pSession->pCallback = NULL;
            FLib_MemCpy(&pSession->remoteAddr, &gCoapDestAddress, sizeof(ipAddr_t));
            COAP_SetUriPath(pSession,(coapUriPath_t *)&gAPP_LED_URI_PATH);

            if(!IP6_IsMulticastAddr(&gCoapDestAddress))
            {
                coapMessageType = gCoapMsgTypeConPost_c;
                pSession->pCallback = APP_CoapGenericCallback;
            }
            else
            {
                APP_ProcessLedCmd(pCommand, dataLen);
            }
            COAP_Send(pSession, coapMessageType, pCommand, dataLen);
        }
    }
    else
    {
        APP_ProcessLedCmd(pCommand, dataLen);
    }
}

/*!*************************************************************************************************
\private
\fn     static void APP_SendLedRgbOn(void *pParam)
\brief  This function is used to send a Led RGB On command over the air.

\param  [in]    pParam    Not used
***************************************************************************************************/
static void APP_SendLedRgbOn
(
    void *pParam
)
{
    uint8_t aCommand[] = {"rgb r000 g000 b000"};
    uint8_t redValue, greenValue, blueValue;

    /* Red value on: 0x01 - 0xFF */
    redValue = (uint8_t)NWKU_GetRandomNoFromInterval(0x01, THR_ALL_FFs8);

    /* Green value on: 0x01 - 0xFF */
    greenValue = (uint8_t)NWKU_GetRandomNoFromInterval(0x01, THR_ALL_FFs8);

    /* Blue value on: 0x01 - 0xFF */
    blueValue = (uint8_t)NWKU_GetRandomNoFromInterval(0x01, THR_ALL_FFs8);

    NWKU_PrintDec(redValue, aCommand + 5, 3, TRUE);     //aCommand + strlen("rgb r")
    NWKU_PrintDec(greenValue, aCommand + 10, 3, TRUE);  //aCommand + strlen("rgb r000 g")
    NWKU_PrintDec(blueValue, aCommand + 15, 3, TRUE);   //aCommand + strlen("rgb r000 g000 b")

    APP_SendLedCommand(aCommand, sizeof(aCommand));
}

/*!*************************************************************************************************
\private
\fn     static void APP_SendLedRgbOff(void *pParam)
\brief  This function is used to send a Led RGB Off command over the air.

\param  [in]    pParam    Not used
***************************************************************************************************/
static void APP_SendLedRgbOff
(
    void *pParam
)
{
    uint8_t aCommand[] = {"rgb r000 g000 b000"};

    APP_SendLedCommand(aCommand, sizeof(aCommand));
}

/*!*************************************************************************************************
\private
\fn     static void APP_SendLedFlash(void *pParam)
\brief  This function is used to send a Led flash command over the air.

\param  [in]    pParam    Not used
***************************************************************************************************/
static void APP_SendLedFlash
(
    void *pParam
)
{
    uint8_t aCommand[] = {"flash"};

    APP_SendLedCommand(aCommand, sizeof(aCommand));
}

/*!*************************************************************************************************
\private
\fn     static void APP_SendLedColorWheel(void *pParam)
\brief  This function is used to send a Led color wheel command over the air.

\param  [in]    pParam    Pointer to stack event
***************************************************************************************************/
static void APP_SendLedColorWheel
(
    void *pParam
)
{
    uint8_t aCommand[] = {"color wheel"};

    APP_SendLedCommand(aCommand, sizeof(aCommand));
}
#endif

/*!*************************************************************************************************
\private
\fn     static void APP_LocalDataSinkRelease(void *pParam)
\brief  This function is used to restore the default destination address for CoAP messages.

\param  [in]    pParam    Pointer to stack event
***************************************************************************************************/
static void APP_LocalDataSinkRelease
(
    void *pParam
)
{
    ipAddr_t defaultDestAddress = APP_DEFAULT_DEST_ADDR;

    FLib_MemCpy(&gCoapDestAddress, &defaultDestAddress, sizeof(ipAddr_t));
    (void)pParam;
}

/*!*************************************************************************************************
\private
\fn     static void APP_CoapLedCb(coapSessionStatus_t sessionStatus, void *pData,
                                  coapSession_t *pSession, uint32_t dataLen)
\brief  This function is the callback function for CoAP LED message.
\brief  It performs the required operations and sends back a CoAP ACK message.

\param  [in]    sessionStatus   Status for CoAP session
\param  [in]    pData           Pointer to CoAP message payload
\param  [in]    pSession        Pointer to CoAP session
\param  [in]    dataLen         Length of CoAP payload
***************************************************************************************************/
static void APP_CoapLedCb
(
    coapSessionStatus_t sessionStatus,
    void *pData,
    coapSession_t *pSession,
    uint32_t dataLen
)
{
    /* Process the command only if it is a POST method */
    if((pData) && (sessionStatus == gCoapSuccess_c) && (pSession->code == gCoapPOST_c))
    {
        APP_ProcessLedCmd(pData, dataLen);
    }

    /* Send the reply if the status is Success or Duplicate */
    if((gCoapFailure_c != sessionStatus) && (gCoapConfirmable_c == pSession->msgType))
    {
        /* Send CoAP ACK */
        COAP_Send(pSession, gCoapMsgTypeAckSuccessChanged_c, NULL, 0);
    }
}

/*!*************************************************************************************************
\private
\fn     static void APP_ProcessLedCmd(uint8_t *pCommand, uint8_t dataLen)
\brief  This function is used to process a LED command (on, off, flash, toggle, rgb, color wheel).

\param  [in]    pCommand    Pointer to command data
\param  [in]    dataLen     Data length
***************************************************************************************************/
static void APP_ProcessLedCmd
(
    uint8_t *pCommand,
    uint8_t dataLen
)
{
    /* Set mode state */
    APP_SetMode(mThrInstanceId, gDeviceMode_Application_c);

    /* Process command */
    if(FLib_MemCmp(pCommand, "on", 2))
    {
        App_UpdateStateLeds(gDeviceState_AppLedOn_c);
    }
    else if(FLib_MemCmp(pCommand, "off", 3))
    {
        App_UpdateStateLeds(gDeviceState_AppLedOff_c);
    }
    else if(FLib_MemCmp(pCommand, "toggle", 6))
    {
        App_UpdateStateLeds(gDeviceState_AppLedToggle_c);
    }
    else if(FLib_MemCmp(pCommand, "flash", 5))
    {
        App_UpdateStateLeds(gDeviceState_AppLedFlash_c);
    }
    else if(FLib_MemCmp(pCommand, "rgb", 3))
    {
        char* p = (char *)pCommand + strlen("rgb");
        uint8_t redValue = 0, greenValue = 0, blueValue = 0;
        appDeviceState_t appState = gDeviceState_AppLedRgb_c;

        dataLen -= strlen("rgb");

        while(dataLen != 0)
        {
            if(*p == 'r')
            {
                p++;
                dataLen--;
                redValue = NWKU_atoi(p);
            }

            if(*p == 'g')
            {
                p++;
                dataLen--;
                greenValue = NWKU_atoi(p);
            }

            if(*p == 'b')
            {
                p++;
                dataLen--;
                blueValue = NWKU_atoi(p);
            }

            dataLen--;
            p++;
        }

        /* Update RGB values */
#if gLedRgbEnabled_d
        Led_UpdateRgbState(redValue, greenValue, blueValue);
#else
        appState = gDeviceState_AppLedOff_c;
        if(redValue || greenValue || blueValue)
        {
            appState = gDeviceState_AppLedOn_c;
        }
#endif
        App_UpdateStateLeds(appState);              
    }
    else if(FLib_MemCmp(pCommand, "color wheel", 11))
    {
#if gLedRgbEnabled_d
        App_UpdateStateLeds(gDeviceState_AppLedColorWheel_c);
#else
        App_UpdateStateLeds(gDeviceState_AppLedFlash_c);
#endif
    }
}

/*!*************************************************************************************************
\private
\fn     static void APP_CoapTempCb(coapSessionStatus_t sessionStatus, void *pData,
                                   coapSession_t *pSession, uint32_t dataLen)
\brief  This function is the callback function for CoAP temperature message.
\brief  It sends the temperature value in a CoAP ACK message.

\param  [in]    sessionStatus   Status for CoAP session
\param  [in]    pData           Pointer to CoAP message payload
\param  [in]    pSession        Pointer to CoAP session
\param  [in]    dataLen         Length of CoAP payload
***************************************************************************************************/
static void APP_CoapTempCb
(
    coapSessionStatus_t sessionStatus,
    void *pData,
    coapSession_t *pSession,
    uint32_t dataLen
)
{
    uint8_t *pTempString = NULL;
    uint32_t ackPloadSize = 0, maxDisplayedString = 10;

    /* Send CoAP ACK */
    if(gCoapGET_c == pSession->code)
    {
        /* Get Temperature */
        pTempString = App_GetTempDataString();
        ackPloadSize = strlen((char*)pTempString);
    }
    /* Do not parse the message if it is duplicated */
    else if((gCoapPOST_c == pSession->code) && (sessionStatus == gCoapSuccess_c))
    {
        if(NULL != pData)
        {
            char addrStr[INET6_ADDRSTRLEN];
            uint8_t temp[10];

            ntop(AF_INET6, &pSession->remoteAddr, addrStr, INET6_ADDRSTRLEN);
            shell_write("\r");

            if(0 != dataLen)
            {
                /* Prevent from buffer overload */
                (dataLen >= maxDisplayedString) ? (dataLen = (maxDisplayedString - 1)) : (dataLen);
                temp[dataLen]='\0';
                FLib_MemCpy(temp,pData,dataLen);
                shell_printf((char*)temp);
            }
            shell_printf("\tFrom IPv6 Address: %s\n\r", addrStr);
            shell_refresh();
        }
    }

    if(gCoapConfirmable_c == pSession->msgType)
    {
        if(gCoapGET_c == pSession->code)
        {
            COAP_Send(pSession, gCoapMsgTypeAckSuccessChanged_c, pTempString, ackPloadSize);
        }
        else
        {
            COAP_Send(pSession, gCoapMsgTypeAckSuccessChanged_c, NULL, 0);
        }
    }

    if(pTempString)
    {
        MEM_BufferFree(pTempString);
    }
}

/*!*************************************************************************************************
\private
\fn     static void APP_CoapSinkCb(coapSessionStatus_t sessionStatus, void *pData,
                                   coapSession_t *pSession, uint32_t dataLen)
\brief  This function is the callback function for CoAP sink message.

\param  [in]    sessionStatus   Status for CoAP session
\param  [in]    pData           Pointer to CoAP message payload
\param  [in]    pSession        Pointer to CoAP session
\param  [in]    dataLen         Length of CoAP payload
***************************************************************************************************/
static void APP_CoapSinkCb
(
    coapSessionStatus_t sessionStatus,
    void *pData,
    coapSession_t *pSession,
    uint32_t dataLen
)
{
    /* Do not execute the command multiple times, if the received message is duplicated */
    if((pData) && (sessionStatus == gCoapSuccess_c))
    {
        /* Process command */
        if(FLib_MemCmp(pData, "create",6))
        {
            /* Data sink create */
            FLib_MemCpy(&gCoapDestAddress, &pSession->remoteAddr, sizeof(ipAddr_t));
        }

        if(FLib_MemCmp(pData, "release",7))
        {
            /* Data sink release */
            APP_LocalDataSinkRelease(NULL);
        }
    }

    if(gCoapConfirmable_c == pSession->msgType)
    {
        /* Send CoAP ACK */
        COAP_Send(pSession, gCoapMsgTypeAckSuccessChanged_c, NULL, 0);
    }
}
/*==================================================================================================
Private debug functions
==================================================================================================*/

/*@Lab - Accelerometer data handling*/
/*!
** @addtogroup APP End Device App
**
** @{
*/
/*!=================================================================================================
\file end_device_app.c
\brief This is a public source file for the end device demo application.
==================================================================================================*/
/*!*************************************************************************************************
\fn static void timerTurnOffLEDsCB(void *param)
\brief This callback is called by the APP_AccCallback function to turn off the LEDs
\param [in] *param Parameter of the callback (not used)
\return void
***************************************************************************************************/
static void timerTurnOffLEDsCB(void *param)
{
	LED_TurnOffAllLeds();
}
/*==================================================================================================
Accelerometer functions
==================================================================================================*/
/*@FX - Accel Interrupt functions and callbacks */
/*!*************************************************************************************************
\private
\fn static void APP_ReportAccel(void *pParam)
\brief This open a socket and report the accelerometer value to gCoapDestAddress.
\param [in] pParam Not used
***************************************************************************************************/
static void APP_ReportAccel
(
		void *pParam
)
{
	coapSession_t *pSession = NULL;
	/* Get Accel */
	uint8_t *pAccelString = App_GetAccelDataString((uint32_t)pParam);
	uint32_t ackPloadSize;
	ifHandle_t ifHandle = THR_GetIpIfPtrByInstId(mThrInstanceId);
	if(!IP_IF_IsMyAddr(ifHandle->ifUniqueId, &gCoapDestAddress))
	{
		pSession = COAP_OpenSession(mAppCoapInstId);
		if(NULL != pSession)
		{
			coapMsgTypesAndCodes_t coapMessageType = gCoapMsgTypeNonPost_c;
			pSession->pCallback = NULL;
			FLib_MemCpy(&pSession->remoteAddr, &gCoapDestAddress, sizeof(ipAddr_t));
			ackPloadSize = strlen((char *)pAccelString);
			COAP_SetUriPath(pSession, (coapUriPath_t *)&gAPP_ACCEL_URI_PATH);
			if(!IP6_IsMulticastAddr(&gCoapDestAddress))
			{
				coapMessageType = gCoapMsgTypeConPost_c;
				pSession->pCallback = APP_CoapGenericCallback;
			}
			COAP_Send(pSession, coapMessageType, pAccelString, ackPloadSize);
		}
	}
	/* Print Accel in shell */
	shell_write("\r");
	shell_write((char *)pAccelString);
	shell_refresh();
	MEM_BufferFree(pAccelString);
}
/*!*************************************************************************************************
\private
\fn static void APP_CoapAccelCb(coapSessionStatus_t sessionStatus, void *pData,
coapSession_t *pSession, uint32_t dataLen)
\brief This function is the callback function for CoAP accelerometer message.
\brief It sends the accelerometer value in a CoAP ACK message.
\param [in] sessionStatus Status for CoAP session
\param [in] pData Pointer to CoAP message payload
\param [in] pSession Pointer to CoAP session
\param [in] dataLen Length of CoAP payload
***************************************************************************************************/
static void APP_CoapAccelCb
(
		coapSessionStatus_t sessionStatus,
		void *pData,
		coapSession_t *pSession,
		uint32_t dataLen
)
{
	uint8_t *pAccelString = NULL;
	uint32_t ackPloadSize = 0, maxDisplayedString = ACCEL_BUFF_SIZE;
	/*Param to know which data will be return from the axis*/
	uint8_t axis_param = 0;
	/* Send CoAP ACK */
	if(gCoapGET_c == pSession->code)
	{
		/* Get Axis */
		if ((FLib_MemCmp(pData, "xyz",3)) || (FLib_MemCmp(pData, "all",3)))
		{
			axis_param = gAccel_All_c;
		}
		else if (FLib_MemCmp(pData, "x",1))
		{
			axis_param = gAccel_X_c;
		}
		else if (FLib_MemCmp(pData, "y",1))
		{
			axis_param = gAccel_Y_c;
		}
		else if (FLib_MemCmp(pData, "z",1))
		{
			axis_param = gAccel_Z_c;
		}
		pAccelString = App_GetAccelDataString((uint32_t)axis_param);
		ackPloadSize = strlen((char*)pAccelString);
	}
	/* Do not parse the message if it is duplicated */
	else if((gCoapPOST_c == pSession->code) && (sessionStatus == gCoapSuccess_c))
	{
		if(NULL != pData)
		{
			char addrStr[INET6_ADDRSTRLEN];
			uint8_t shellStr[ACCEL_BUFF_SIZE];
			ntop(AF_INET6, &pSession->remoteAddr, addrStr, INET6_ADDRSTRLEN);
			shell_write("\r");
			if(0 != dataLen)
			{
				/* Prevent from buffer overload */
				(dataLen > maxDisplayedString) ? (dataLen = maxDisplayedString) : (dataLen);
				shellStr[dataLen]='\0';
				FLib_MemCpy(shellStr,pData,dataLen);
				shell_printf((char*)shellStr);
			}
			shell_printf("\tFrom IPv6 Address: %s\n\r", addrStr);
			shell_refresh();
		}
	}
	if(gCoapConfirmable_c == pSession->msgType)
	{
		if(gCoapGET_c == pSession->code)
		{
			COAP_Send(pSession, gCoapMsgTypeAckSuccessChanged_c, pAccelString, ackPloadSize);
		}
		else
		{
			COAP_Send(pSession, gCoapMsgTypeAckSuccessChanged_c, NULL, 0);
		}
	}
}

/*!*************************************************************************************************
\fn static void Accel_Callback(uint8_t events)
\brief This is a callback is passed to the Accelerometer initialization to store the accelerometer
interrupt events.
\param [in] events value of the events
\return void
***************************************************************************************************/
static void Accel_Callback(uint8_t events)
{
	if((mAccLastEvent == gAppAccNoEvent_c))
	{
		mAccLastEvent = events;
		APP_AccCallback();
	}
}
/*!*************************************************************************************************
\fn static void APP_AccCallback(void)
\brief This is a callback function called from Accel_Callback
\return void
***************************************************************************************************/
static void APP_AccCallback(void)
{
	uint8_t redValue = 0, greenValue = 0, blueValue = 0;
	TMR_StopTimer(mLEDOffTimerID);
	switch(mAccLastEvent)
	{
	case gAccel_X_c:
		NWKU_SendMsg(APP_ReportAccel, (void*)gAccel_X_c, mpAppThreadMsgQueue); //Send X Accel
		redValue = 255;
		break;
	case gAccel_Y_c:
		NWKU_SendMsg(APP_ReportAccel, (void*)gAccel_Y_c, mpAppThreadMsgQueue); //Send Y Accel
		greenValue = 255;
		break;
	case gAccel_Z_c:
		NWKU_SendMsg(APP_ReportAccel, (void*)gAccel_Z_c, mpAppThreadMsgQueue); //Send Z Accel
		blueValue = 255;
		break;
	default:
		break;
	}
	Led_UpdateRgbState(redValue, greenValue, blueValue);
	App_UpdateStateLeds(gDeviceState_AppLedRgb_c);
	mAccLastEvent = gAppAccNoEvent_c;
	TMR_StartSingleShotTimer(mLEDOffTimerID, 100, timerTurnOffLEDsCB, NULL);
}




static void APP_CoapEquipo4Cb(coapSessionStatus_t sessionStatus,void *pData,coapSession_t *pSession,uint32_t dataLen){

	char addrStr[INET6_ADDRSTRLEN];
	uint8_t shellStr[ACCEL_BUFF_SIZE];
	ntop(AF_INET6, &pSession->remoteAddr, addrStr, INET6_ADDRSTRLEN);
	shell_write("\r");

	if(0 != dataLen)
	{
		/* Prevent from buffer overload */
		shellStr[dataLen]='\0';
		FLib_MemCpy(shellStr,pData,dataLen);
		//shell_printf((char*)shellStr);
	}
	shell_printf("\t Counter %s From IPv6 Address: %s\n\r",(char*)shellStr, addrStr);
	shell_refresh();

	if(gCoapGET_c == pSession->code)
	{
		/* Get Axis */


	}
	/* Do not parse the message if it is duplicated */
	else if((gCoapPOST_c == pSession->code) && (sessionStatus == gCoapSuccess_c))
	{

	}
	if(gCoapConfirmable_c == pSession->msgType)
	{

	}
}

static void APP_TimmerEquipo4Cb(void *pParam){
	TMR_StopTimer(mEquipo4GETTimerID);
	//coapSession_t *pSession = NULL;
	    /* Get Temperature */
	   uint8_t *pTempString = "algo";
	   /*  uint32_t ackPloadSize;
	    ifHandle_t ifHandle = THR_GetIpIfPtrByInstId(mThrInstanceId);

	    if(!IP_IF_IsMyAddr(ifHandle->ifUniqueId, &gCoapDestAddress))
	    {
	        pSession = COAP_OpenSession(mAppCoapInstId);

	        if(NULL != pSession)
	        {
	            coapMsgTypesAndCodes_t coapMessageType = gCoapMsgTypeConGet_c;

	            pSession->pCallback = NULL;
	            FLib_MemCpy(&pSession->remoteAddr, &gCoapDestAddress, sizeof(ipAddr_t));
	            ackPloadSize = strlen((char *)pTempString);
	            COAP_SetUriPath(pSession, (coapUriPath_t *)&gAPP_EQUIPO4_URI_PATH);

	            if(!IP6_IsMulticastAddr(&gCoapDestAddress))
	            {
	                coapMessageType = gCoapMsgTypeConPost_c;
	                pSession->pCallback = APP_CoapGenericCallback;
	            }

	            COAP_Send(pSession, coapMessageType, pTempString, ackPloadSize);
	        }
	    }
	    */
	    /* Print temperature in shell */
	    shell_write("\r");
	    shell_printf("%s \r\n",(char *)pTempString);
	    shell_refresh();
	    MEM_BufferFree(pTempString);

	    TMR_StartSingleShotTimer(mEquipo4GETTimerID, 1000, APP_TimmerEquipo4Cb, NULL);
}
