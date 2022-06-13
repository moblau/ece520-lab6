/*
 ************************************************************************************
 *
 * stopwatch_intr.c
 *
 * completed code snippets...
 *    Enable the SCU GIC interrupt here ->
 *    XScuGic_Enable(&InterruptController, XPAR_XTTCPS_0_INTR);
 *
 *    Enable the timer interrupt here ->
 *    XTtcPs_EnableInterrupts(&TtcPsInst, XTTCPS_IXR_INTERVAL_MASK);
 *
 * History
 *    2022/01/21 - wk     - 2021.2, major re-write to restructure and modernize
 *    2021/09/14 - wk     - 2021.1, code cleanup and addition of Versal support
 *    2020/03/06 - wk, am - update for 2019.2 - this program has been around for about
 *                          while, but no-one decided to put a header on it so we can't\r
 *                          track its history.
 *
 ************************************************************************************
 */

#include <stdio.h>
#include <stdlib.h>
#include "xttcps.h"
#include "xscugic.h"
#include "xgpio.h"
#if (CPU_TYPE==72)
#include "xuartpsv.h"
#elif (CPU_TYPE==53)
#include "xuartps.h"
#endif

#define PRINT printf

#if (CPU_TYPE==72)
XUartPsv PSuart; /* The instance of the UART Driver */
#elif (CPU_TYPE==53)
XUartPs PSuart; /* The instance of the UART Driver */
#endif

static volatile u8 UpdateFlag;   /* Flag to update the seconds counter */
static u32 TickCount;      /* Ticker interrupts between seconds change */

static void TickHandler(void *CallBackRef);
void dumpOptions() { PRINT("Usage:\n\tr=RESET\n\ts=STOP\n\tp=PAUSE\n\tg=GO\n"); }

int main(void) {
   // 1 - Set up the interrupt configuration
   XGpio push;
   u32 psb_check;

   XGpio_Initialize(&push, XPAR_AXI_GPIO_0_DEVICE_ID);
   XGpio_SetDataDirection(&push,1,0xFFFFFFFF);







#if (CPU_TYPE==72)
    XUartPsv_Config *uConfig;
#elif (CPU_TYPE==53)
   XUartPs_Config *uConfig;
#endif

   int Status;
   XScuGic InterruptController; //IntcInstancePtr
   XScuGic_Config *IntcConfig;  // The configuration parameters of the interrupt controller

   // welcome the user
   PRINT("\n************************************************************\n");
   PRINT("*                  StopWatch Application                   *\n");
   PRINT("*                    INTERRUPT Driven                      *\n");
   PRINT("************************************************************\n\n");

   // 1 setting up interrupts

   // Initialize the interrupt controller driver
   //manage the SCU's GIC
   IntcConfig = XScuGic_LookupConfig(XPAR_SCUGIC_SINGLE_DEVICE_ID);
   if (NULL == IntcConfig) { return XST_FAILURE; }

   Status = XScuGic_CfgInitialize(&InterruptController, IntcConfig,IntcConfig->CpuBaseAddress);
   if (Status != XST_SUCCESS) { return XST_FAILURE; }

   /* Connect the interrupt controller interrupt handler to the hardware
    * interrupt handling logic in the ARM processor.
    */
   Xil_ExceptionRegisterHandler(XIL_EXCEPTION_ID_IRQ_INT,(Xil_ExceptionHandler) XScuGic_InterruptHandler,&InterruptController);

   // Enable interrupts in the ARM
   Xil_ExceptionEnable();

   // 2 - Set up  the Ticker timer
   XTtcPs_Config *Config;
   XTtcPs TtcPsInst;    //Timer
   u32 OutputHz = 100;  // Output frequency
   XInterval Interval;  // Interval value
   u8 Prescaler;        // Prescaler value

   // Look up the configuration based on the device identifier

   //Manage the Timer
   Config = XTtcPs_LookupConfig(XPAR_XTTCPS_0_DEVICE_ID);
   if (NULL == Config) { return XST_FAILURE; }

   // Initialize the device
   Status = XTtcPs_CfgInitialize(&TtcPsInst, Config, Config->BaseAddress);
   if (Status != XST_SUCCESS) { return XST_FAILURE; }

   // Set the options
   XTtcPs_SetOptions(&TtcPsInst,XTTCPS_OPTION_INTERVAL_MODE | XTTCPS_OPTION_WAVE_DISABLE);

   /*
    * Timer frequency is preset in the TimerSetup structure,
    * however, the value is not reflected in its other fields, such as
    * IntervalValue and PrescalerValue. The following call will map the
    * frequency to the interval and prescaler values.
    */
   XTtcPs_CalcIntervalFromFreq(&TtcPsInst, OutputHz, &Interval, &Prescaler);

   // Set the interval and prescaler
   XTtcPs_SetInterval(&TtcPsInst, Interval);
   XTtcPs_SetPrescaler(&TtcPsInst, Prescaler);

   // Connect to the interrupt controller
   Status = XScuGic_Connect(&InterruptController, XPAR_XTTCPS_0_INTR,(Xil_InterruptHandler) TickHandler, (void *) &TtcPsInst);
   if (Status != XST_SUCCESS) { return XST_FAILURE; }

   // *** Enable the SCU GIC interrupt here
  // enable the GIC here
   XScuGic_Enable(&InterruptController, XPAR_XTTCPS_0_INTR);
   /*
    * Enable the interrupts for the tick timer/counter
    * We only care about the interval timeout.
    */
   // *** Enable the timer interrupt here
  // enable the interrupts to the PS here
   XTtcPs_EnableInterrupts(&TtcPsInst, XTTCPS_IXR_INTERVAL_MASK);
   /*
    * Initialize the UART driver so that it's ready to use
    * Look up the configuration in the config table and then initialize it.
    */
#if (CPU_TYPE==72)
   uConfig = XUartPsv_LookupConfig(XPAR_XUARTPSV_0_DEVICE_ID);
   if (NULL == uConfig) { return XST_FAILURE; }
   Status = XUartPsv_CfgInitialize(&PSuart, uConfig, uConfig->BaseAddress);
   XUartPsv_SetBaudRate(&PSuart, 115200);
#elif (CPU_TYPE==53)
    uConfig = XUartPs_LookupConfig(XPAR_XUARTPS_0_DEVICE_ID);
   if (NULL == uConfig) { return XST_FAILURE; }
   Status = XUartPs_CfgInitialize(&PSuart, uConfig, uConfig->BaseAddress);
   XUartPs_SetBaudRate(&PSuart, 115200);
#endif

   // tell the user the available choices:
   dumpOptions();

   // define the time fields
   u8  hsecs           = 0;
   u8  seconds         = 0;
   u32 minutes         = 0;

   // variable for getting user input
   u8  readVal;

   while (1) {
#if (CPU_TYPE==72)
      //XUartPsv_Recv(&PSuart,&readVal,1);
#elif (CPU_TYPE==53)
      XUartPs_Recv(&PSuart,&readVal,1);
#endif

      // decide what to do with the character
      psb_check = XGpio_DiscreteRead(&push,1);
      switch (psb_check) {
         case 0x01:               // "go" (start)
            if (XTtcPs_IsStarted(&TtcPsInst) == 0) {
               XTtcPs_Start(&TtcPsInst);
            }
            break;

         case 0x02:               // "pause" (pauses current count without changing the value of the timer)
            XTtcPs_Stop(&TtcPsInst);
            PRINT("Paused at %d:%02d.%02d\r",minutes,seconds,hsecs);
            break;

         case 0x04:               // "stop" (stops the timer and resets)
            XTtcPs_Stop(&TtcPsInst);
            minutes=0; seconds=0; hsecs=0;
            PRINT("Stopped and reset\r");
            UpdateFlag = 0;      // prevent any extra "tics" from getting through...
            break;

         case 0x08:               // "reset" (drives the timer counters to zero) - even when running
            minutes=0; seconds=0; hsecs = 0;
            break;

         case 15:               // display commands
            dumpOptions();
            break;

         default:
            ; // ignore all other received characters
      }

      if (UpdateFlag) {
       UpdateFlag = 0;        // clear the flag set by the handler

         // Calculate the time setting here, not at the time critical interrupt level.
         hsecs++;                   // increment the hundredths of a second counter
         if (hsecs == 100) {        // is it time to roll over to a full second?
            hsecs = 0;              // roll-over the hundredths of a second counter
            seconds++;              // and increment the seconds counter
            if (seconds == 60) {    // is it time to roll over the seconds counter?
               seconds = 0;         // roll-over the seconds counter
               minutes++;           // and increment the minutes counter
            }
         }
         PRINT("Running: %d:%02d.%02d\r",minutes,seconds,hsecs);
      }

#ifndef QEMU
         fflush(stdout);
#endif

   }
   return XST_SUCCESS;
}

/**
 * TickHandler
 *    Interrupt handler for the Timer
 *    This is a minimal impact handler and is designed to be brief. In short, it gets the ID of the interrupt
 *    that caused this routine to be called, then clears it to avoid constant (and erroneous) activation. It then
 *    check that the triple-timer counter interval mask is set and increments the TickCount.
 *    A bit of protection is included here in that if the TickCount wasn't cleared by the main routine, TickCount
 *    will continue to increment and additional diagnostic code could be added here and triggered if TickCount is
 *    greater than 1 indicating that the main routine wasn't fast enough in clearing the flag. As the code
 *    is now, TickCount is immediately reset to zero.
 *    A more efficient coding, once the main was proven to handle the UpdateFlag in a timely fashion, would
 *    be to remove TickCount altogether and just set UpdateFlag.
 */
static void TickHandler(void *CallBackRef){
   u32 StatusEvent;

  // Read the interrupt status, then write it back to clear the interrupt.
  StatusEvent = XTtcPs_GetInterruptStatus((XTtcPs *)CallBackRef);
  XTtcPs_ClearInterruptStatus((XTtcPs *)CallBackRef, StatusEvent);

  if (0 != (XTTCPS_IXR_INTERVAL_MASK & StatusEvent)) {
    TickCount++;

    if (TickCount) {
      TickCount = 0;
      UpdateFlag = TRUE;
    }
  }
}



