# Lab 6 - Creating a Stopwatch using TTC, UART and Interrupts
## 6.1 Introduction
Lab 6 is an introduction to the Zynq Triple Timer Counter. The Zynq TTC is a flexible timing resource for SoC designs. In the case for lab 6, the TTC was used to implement a simple stop watch with the Zynq 7 processor. Lab 6 contains two parts:

### Part 1
Implement a stopwatch using the TTC, UART and Interrupts with QEMU, a generic and open source machine emulator and virtualizer. The C source code is given as part of the assignment. The user controls the stopwatch by interacting with the UART terminal. 

### Part 2
Implement a stopwatch using the TTC, UART and Interrupts on the Zybo Z7 SoC development board. This part also requires the given C source code to be slightly modified so that the stopwatch can operate by pressing buttons on the Zybo. The code modifications were derived from the work done in Lab 4.

## Procedure
### Part 1
The firmware .xsa file for the emulated board in QEMU was given. Thus, this part of the lab was entirely done with Vitis. The TCL command line was used to create the Vitis project files. The following two lines of code were missing from the given source code, and had to be added:
 c code
```C
//Enable the SCU GIC Interrupt
XScuGic_Enable(&InterruptController, XPAR_XTTCPS_0_INTR); 

//Enable the interrupts to the PS
XTtcPs_EnableInterrupts(&TtcPsInst, XTTCPS_IXR_INTERVAL_MASK);
```

The QEMU console was opened upon running the program as a software emulation. The stopwatch worked in the QEMU console by receiving character inputs from the user.

###Part 2
The firmware for part 2 needed to be created to use the TTC on the Zybo. In Vivado, a block design was created with TTC and UART enabled. An AXI GPIO was added to connect the PS to the push buttons. Once the top level design was synthesized, the TTC output was mapped via I/O mapping to the R19 register. This allowed the PS to receive the TTC waveforms from the PL. 

The generated .xsa was imported to Vitis. The given source code was modified to include the use of the switch buttons to control the stopwatch by adding the following lines:

```C
XGpio push;
u32 psb_check;

XGpio_Initialize(&push, XPAR_AXI_GPIO_0_DEVICE_ID);
XGpio_SetDataDirection(&push,1,0xFFFFFFFF);
```

In the while loop of the main function, the push button data was read and the switch statement was changed to react to the 4 states of the push buttons: 0x1,0x2,0x4,0x8. The project and program were built and programmed to the FPGA. The Vitis serial terminal was used to monitor the UART data stream from the Zybo.

## Results

**Part 1** results can be seen [here](https://youtu.be/-FQ72GkDynI)

**Part 2** results can be seen [Here](https://youtube.com/shorts/P1whh6pWBTo?feature=share)

## Issues
- I was missing the necessary linux net-tools libary, which gave me issues when using QEMU. After installing the net-tools library, this issue was fixed.
- Part 1 stopped working after running the program in the QEMU console several times. I tried getting it to work again but I could not figure it out. Thankfully, I already had a video of it working.
- For Part 2, I initially forget to include the two additional lines of code to initialize the interrupts. There was repsonse from the console but the stopwatch wasn't working. After troubleshooting, I realized what was mising and after adding the two lines of code, it was able to work.
- 
## Conclusion
This lab helped me practice the workflow that is used to use the Zybo Z7 SoC development board. I am more comfortable using the block design editor in Vivado and coding in C in Vitis. It is exciting to see the basics of how these embedded systems can be developed.

