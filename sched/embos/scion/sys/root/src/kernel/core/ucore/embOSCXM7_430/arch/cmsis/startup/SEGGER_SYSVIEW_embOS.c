/*********************************************************************
*                SEGGER Microcontroller GmbH & Co. KG                *
*                        The Embedded Experts                        *
**********************************************************************
*                                                                    *
*       (c) 1995 - 2016 SEGGER Microcontroller GmbH & Co. KG         *
*                                                                    *
*       Internet: segger.com  Support: support_embos@segger.com      *
*                                                                    *
**********************************************************************
*                                                                    *
*       embOS * Real time operating system for microcontrollers      *
*                                                                    *
*       Please note:                                                 *
*                                                                    *
*       Knowledge of this file may under no circumstances            *
*       be used to write a similar product or a real-time            *
*       operating system for in-house use.                           *
*                                                                    *
*       Thank you for your fairness !                                *
*                                                                    *
**********************************************************************
*                                                                    *
*       OS version: 4.30                                             *
*                                                                    *
**********************************************************************

-------------------------- END-OF-HEADER -----------------------------

File    : SEGGER_SYSVIEW_embOS.c
Purpose : Interface between embOS and System View.
Revision: $Rev: 3745 $
*/

#include "RTOS.h"
#include "SEGGER_SYSVIEW.h"
#include "SEGGER_SYSVIEW_embOS.h"
#include "SEGGER_RTT.h"

#if (OS_VERSION < 41201)
  #error "SystemView is only supported in embOS V4.12a and later."
#endif

static void _cbSendTaskInfo(const OS_TASK* pTask) {
  SEGGER_SYSVIEW_TASKINFO Info;

  OS_EnterRegion();         // No scheduling to make sure the task list does not change while we are transmitting it
  memset(&Info, 0, sizeof(Info));     // Fill all elements with 0 to allow extending the structure in future version without breaking the code
  Info.TaskID = (U32)pTask;
#if OS_TRACKNAME
  Info.sName = pTask->Name;
#endif
  Info.Prio = pTask->Priority;
#if OS_CHECKSTACK
  Info.StackBase = (U32)pTask->pStackBot;
  Info.StackSize = pTask->StackSize;
#endif
  SEGGER_SYSVIEW_SendTaskInfo(&Info);
  OS_LeaveRegion();         // No scheduling to make sure the task list does not change while we are transmitting it
}

/*********************************************************************
*
*       OS_SYSVIEW_SendTaskList()
*
*  Function description
*    This function is part of the link between embOS and SYSVIEW.
*    Called from SystemView when asked by the host, it uses SYSVIEW
*    functions to send the entire task list to the host.
*/
static void _cbSendTaskList(void) {
  OS_TASK * pTask;

  OS_EnterRegion();         // No scheduling to make sure the task list does not change while we are transmitting it
  for (pTask = OS_Global.pTask; pTask; pTask = pTask->pNext) {
    _cbSendTaskInfo(pTask);
  }
  OS_LeaveRegion();         // No scheduling to make sure the task list does not change while we are transmitting it
}

static void _cbRecordU32(unsigned Id, OS_U32 Para0) {
  SEGGER_SYSVIEW_RecordU32  (Id, Para0);
}
static void _cbRecordU32x2(unsigned Id, OS_U32 Para0, OS_U32 Para1) {
  SEGGER_SYSVIEW_RecordU32x2(Id, Para0, Para1);
}
static void _cbRecordU32x3(unsigned Id, OS_U32 Para0, OS_U32 Para1, OS_U32 Para2) {
  SEGGER_SYSVIEW_RecordU32x3(Id, Para0, Para1, Para2);
}

static void _cbRecordU32x4(unsigned Id, OS_U32 Para0, OS_U32 Para1, OS_U32 Para2, OS_U32 Para3) {
  SEGGER_SYSVIEW_RecordU32x4(Id, Para0, Para1, Para2, Para3);
}
static void _cbRecordU32x5(unsigned Id, OS_U32 Para0, OS_U32 Para1, OS_U32 Para2, OS_U32 Para3, OS_U32 Para4) {
  SEGGER_SYSVIEW_RecordU32x5(Id, Para0, Para1, Para2, Para3, Para4);
}

static OS_U32 _ShrinkId(OS_U32 Ptr) {
  return (OS_U32)SEGGER_SYSVIEW_ShrinkId(Ptr);
}

static void _RecordEnterTimer(OS_U32 TimerID) {
  SEGGER_SYSVIEW_RecordEnterTimer(TimerID);
}

static void _RecordExitTimer(void) {
  SEGGER_SYSVIEW_RecordExitTimer();
}

static void _RecordEndCallU32(unsigned int Id, OS_U32 RetVal) {
  SEGGER_SYSVIEW_RecordEndCallU32(Id, RetVal);
}

// embOS trace API that targets SYSVIEW
const OS_TRACE_API embOS_TraceAPI_SYSVIEW = {
//
// Specific Trace Events
//
SEGGER_SYSVIEW_RecordEnterISR,                //  void (*pfRecordEnterISR)        (void);
SEGGER_SYSVIEW_RecordExitISR,                 //  void (*pfRecordExitISR)         (void);
SEGGER_SYSVIEW_RecordExitISRToScheduler,      //  void (*pfRecordExitISRToScheduler)  (void);
_cbSendTaskInfo,                              //  void (*pfRecordTaskInfo)        (const OS_TASK* pTask);
SEGGER_SYSVIEW_OnTaskCreate,                  //  void (*pfRecordTaskCreate)      (unsigned TaskId);
SEGGER_SYSVIEW_OnTaskStartExec,               //  void (*pfRecordTaskStartExec)   (unsigned TaskId);
SEGGER_SYSVIEW_OnTaskStopExec,                //  void (*pfRecordTaskStopExec)    (void);
SEGGER_SYSVIEW_OnTaskStartReady,              //  void (*pfRecordTaskStartReady)  (unsigned TaskId);
SEGGER_SYSVIEW_OnTaskStopReady,               //  void (*pfRecordTaskStopReady)   (unsigned TaskId, unsigned Reason);
SEGGER_SYSVIEW_OnIdle,                        //  void (*pfRecordIdle)            (void);
//
// Generic Trace Event logging
//
SEGGER_SYSVIEW_RecordVoid,                    //  void    (*pfRecordVoid)     (unsigned Id);
_cbRecordU32,                                 //  void    (*pfRecordU32)      (unsigned Id, OS_U32 Para0);
_cbRecordU32x2,                               //  void    (*pfRecordU32x2)    (unsigned Id, OS_U32 Para0, OS_U32 Para1);
_cbRecordU32x3,                               //  void    (*pfRecordU32x3)    (unsigned Id, OS_U32 Para0, OS_U32 Para1, OS_U32 Para2);
_cbRecordU32x4,                               //  void    (*pfRecordU32x4)    (unsigned Id, OS_U32 Para0, OS_U32 Para1, OS_U32 Para2, OS_U32 Para3);
_ShrinkId,                                    //  OS_U32  (*pfPtrToId)        (OS_U32 Ptr);
#if (OS_VERSION >= 41400)  // Tracing timer is supported since embOS V4.14
_RecordEnterTimer,                            //  void    (*pfRecordEnterTimer)       (OS_U32 TimerID);
_RecordExitTimer,                             //  void    (*pfRecordExitTimer)        (void);
#endif
#if (OS_VERSION >= 42400)   // Tracing end of call supported since embOS V4.24
SEGGER_SYSVIEW_RecordEndCall,                 //  void    (*pfRecordEndCall)             (unsigned int Id);
_RecordEndCallU32,                            //  void    (*pfRecordEndCallReturnValue)  (unsigned int Id, OS_U32 ReturnValue);
SEGGER_SYSVIEW_OnTaskTerminate,               //  void    (*pfRecordTaskTerminate)       (unsigned TaskId);
_cbRecordU32x5,                               //  void    (*pfRecordU32x5)               (unsigned Id, OS_U32 Para0, OS_U32 Para1, OS_U32 Para2, OS_U32 Para3, OS_U32 Para4);
#endif
};

// Services provided to SYSVIEW by embOS
const SEGGER_SYSVIEW_OS_API SYSVIEW_X_OS_TraceAPI = {
  OS_GetTime_us64,
  _cbSendTaskList,
};

/*************************** End of file ****************************/
