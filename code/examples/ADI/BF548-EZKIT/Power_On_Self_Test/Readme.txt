"ADSP-BF548 EZ-KIT Lite POST TEST and Loader File Demo"

Date Created:	5/22/07		Rev 1.0

NOTE: The Power_On_Self_Test is too large of an example to work with a DEMO license
and can only be run in it's entirety using a full VDSP++ license.

WARNING: Because the hard drive test currently writes and verifies raw data,
		 the hard drive will need to be reformatted after running the test.
		 
This directory contains an example ADSP-BF548 project which demonstrates how
to create a loader file for the BF548 EZ-KIT and use the FlashProgrammer utility
to download the file to the P28F128P33 flash.

Files contained in this directory:

+Power_On_Self_Test
¦   adi_otp.c
¦   adi_otp.h
¦   adi_ssl_Init.c
¦   adi_ssl_Init.h
¦   Audio_test.c
¦   AV_Audio_test.c
¦   AV_Video_test.c
¦   CAN_test.c
¦   DDR_test.c
¦   DisplayPics.c
¦   endec_64.c
¦   ethernet_test.c
¦   ethernet_test.h
¦   HDD_test.c
¦   image1.dat
¦   image2.dat
¦   image3.dat
¦   Keypad_test.c
¦   LCD.h
¦   low_level_api.h
¦   m25p16_test.c
¦   MACaddress.c
¦   main.c
¦   mxvr_test.c
¦   mxvr_test.h
¦   NAND_test.c
¦   otp_helper_functions.c
¦   otp_helper_functions.h
¦   otp_low_level_api.asm
¦   OTP_publickey.c
¦   PBLED_test.c
¦   PBLED_test.h
¦   PC28F128_common.c
¦   PC28F128_common.h
¦   PC28F128_test.c
¦   post_common.h
¦   Power_On_Self_Test.dpj
¦   Power_On_Self_Test.ldf
¦   Power_On_Self_Test.ldr
¦   Power_On_Self_Test.mak
¦   Power_On_Self_Test_basiccrt.s
¦   Power_On_Self_Test_heaptab.c
¦   ProcessorVersion_test.c
¦   public_key.dat
¦   Readme.txt
¦   Rotary_test.c
¦   RTC_test.c
¦   SD_test.c
¦   SD_test.h
¦   temp.txt
¦   Timer_ISR.c
¦   Timer_ISR.h
¦   TouchLCD_test.c
¦   UART_test.c
¦   USB_common.c
¦   USB_common.h
¦   USB_test.c
¦
+---test_FPGA
        fpga.bit
        fpga.hex
        fpga.mcs
        FPGA_gpio_test.c
        fpga_image.h
        FPGA_memory_test.c
        FPGA_sports_test.c
        ImageConverter.exe
        ProgramFPGA.c

__________________________________________________________

	

CONTENTS

I.	FUNCTIONAL DESCRIPTION
II.	IMPLEMENTATION DESCRIPTION
III. 	OPERATION DESCRIPTION
IV.	POST NOTES


I.    FUNCTIONAL DESCRIPTION

The Loader File demo demonstrates how to create a loader file, as well as
provides a Power On Self Test (POST) for the EZ-Kit Lite board.
 
After the loader file is created the user can then use the FlashProgrammer Utility
to download the intel hex file to the P28F128P33 flash.


II.   IMPLEMENTATION DESCRIPTION

Boots a blink program or POST from Flash and runs a simple blink program.

III.  OPERATION DESCRIPTION

The switch settings for the built in self test of the ADSP-BF548 EZ-KIT Lite differ slightly from the
default settings shipped.  See the users manual for default switch settings.
EZ-KIT Lite switch and jumper settings:

	- SW1	should be set to position 1
	- SW2	1 = ON,  2 = ON,  3 = ON,  4 = ON  5 = ON, 6 = ON, 7 = ON,  8 = ON.
	- SW4	1 = ON,  2 = ON,  3 = ON,  4 = OFF.
	- SW5	1 = ON,  2 = ON,  3 = ON,  4 = ON.
	- SW7	1 = ON,  2 = ON,  3 = ON,  4 = OFF.
	- SW8	1 = ON,  2 = ON,  3 = ON,  4 = ON, 5 = ON, 6 = ON.
	- SW14	1 = ON,  2 = ON,  3 = ON,  4 = ON.
	- SW16	1 = ON,  2 = ON,  3 = ON,  4 = OFF.
	- SW17	1 = ON,  2 = OFF, 3 = ON,  4 = OFF.
	
	- JP1: installed
	- JP2: NOT installed
	- JP3: NOT installed
	- JP4: installed
	- JP5: installed
	- JP6: installed
	- JP7: NOT installed
	- JP8: NOT installed
	- JP11: 1&2 installed
	        3&4 installed (right to left with SPORT 2 connector below)

	
	
"Creating a Loader File for the ADSP-BF548 EZ-KIT"

- Open an ADSP-BF548 EZ-KIT Lite session in the VisualDSP Integrated Development Environment (IDDE).
- Open the project "Power_On_Self_Test.dpj" in the VisualDSP IDDE.
- Click on the "Project" Menu item and select "Project Options"
- Select "Project" in the left hand window.
- Make sure that Type is set to Loader file
- Select the "Project:Load:Options" from the left hand window
- Under "Boot Mode", "Flash/PROM" is selected
- "Boot Format" should be "Intel hex"
- "Output Width" should be "16"
- Use default start address should be checked unless the start address should be something
  other than the beginning of flash memory
- "Verbose" should be unchecked
- Since we are using external memory, the initialization file should point to
  ..\Blackfin\ldr\INIT_CODE.DXE
- "Output file" and "Additional options" should be blank. 
- After all the options are set up select "OK"
- Click the "Project" menu item and select "rebuild all" to build "Power_On_Self_Test.ldr"
- "Power_On_Self_Test.ldr" should now be in your Project folder


DOWNLOADING "Power_On_Self_Test.ldr" IN THE FLASH PROGRAMMER

- Click on the "Tools" menu item and select "Flash Programmer..."
- The Flash Programmer interface will pop up
- Click on the "Browse..." button next to the "Load Driver" button to browse for the driver
- The driver should be in 
  "...\Blackfin\Examples\ADSP-BF548 EZ-KIT Lite\Flash Programmer\Burst"
- Select the driver file "BF548EzFlashDriver_PC28f128.dxe" and then click "Load Driver"
- Click on "Programming..." tab 
- Click on the "Browse..." button for "Data file" to browse for Power_On_Self_Test.ldr
- Select the Power_On_Self_Test.ldr file just created and click "Program"
- "Power_On_Self_Test.ldr" is now in flash
- Close the Flash Programmer dialog box.


- To verify that the POST program has been written to flash, configure the board to
  boot from the the PC28F128P33 flash.
  - Set SW1 to position 1
- Shutdown VisualDSP, press the reset button on the EZ-Kit Lite board, the LED bank should 
  begin blinking.
  
  
NOTE:  Alternate method for users of the EZ-Kit Lite onboard debug agent.
The above steps will work, however for your convenience, there is an alternate method
  of resetting the board and performing a boot.
  
- To verify that the blink program is in flash go to the Settings menu and
  click BootLoad.  This will cause the board to boot from flash and run.
  You should see your leds flashing.
- You can halt the dsp at any time by clicking on Debug\halt.
- You can load the symbols of Power_On_Self_Test.ldr using the "File Load Symbols" menu item and 
  selecting the Power_On_Self_Test.ldr file, this will not change the state of the processor from where 
  it was halted, but it will correlate debug information to what was booted from the flash.
  
 IV.  POST TEST NOTES
 
 The following explains the use of the POST for the ADSP-BF548 EZ-KIT Lite.  These instructions
 are based on having loaded the P28F128P33 flash with the ldr file created in the steps above and
 the user is booting the POST from the flash device.
 
 Please refer to the table at the beginning of main.c for the LED pattern used for each
 test.
 
 Normal Tests:
  - Power up BF548 EZ-kit.
  - Hold SW13(PB1) push button while pressing reset button.
  - Release the reset button continuing to hold down SW13 until LED1 lights up.
  - Wait for LED1 to go out.
  - Press each push button for the push button test.
  - When LED3 comes on by itself, press each key on the keypad.
  - Once the LEDs are blinking left to right turn the rotary switch clockwise until the LEDs go 
    out making sure they are moving faster and faster as you turn.  Also this test may go really 
    fast and then back to a normal speed.  Keep turning still until they go out.
  - Push the Rotary switch and LEDs should reset to normal speed
  - Turn Rotary switch counter clockwise until they are all on or the next test is started
  - Next 9 boxes should appear on the LCD.  Make sure the top row is red, the middle is green, and 
    the last is blue.
  - Press each box with your finger and it should turn white when you press it.  You can also
    keep pressure on the screen and move your finger to touch each box without removing it
    from the screen at any point in time.
  - POST test program goes into auto test mode.
 
 Public Key Programming:
  - Power up BF548 EZ-kit.
  - Hold SW12(PB2) and SW11(PB3) while pressing the reset button.
  - Release the reset button continuing to hold down SW12 and SW11 until LED2 and LED4 light up.
  - The normal blink program will be run after the test is complete.
  - If LED2 and LED4 are blinking very fast then this failed.
 
 MAC Address Test:
  - Power up BF548 EZ-kit.
  - Plug in the USB cable to USB_OTG(P4) and a USB port on the laptop.
  - Hold SW11(PB3) and SW10(PB4) push while pressing the reset button.
  - Release the reset button continuing to hold down SW11 and SW10 until LED4 and LED1 light up.
  - Open a DOS command prompt to ..\VisualDSP 5.0\Blackfin\Examples\usb\host\windows\hostapp
  - Type hostapp.exe -x 0 <last 2 bytes of MAC ADDRESS>.  We specifically OR in the rest of the
    MAC ADDRESS that has been allocated to Analog Devices.  If a user needs to enter a MAC ADDRESS
    other than and Analog Devices provided one, then the code would need to be altered.
  - Ex. hostapp.exe -x 0 1122
  - To verify what was written type hostapp.exe -x 1

 USB Tests:
  - Power up BF548 EZ-kit.
  - Hold SW12(PB2) push button while pressing reset button.
  - Release the reset button continuing to hold down SW12 until LED1, LED2, and LED3 lights up.
  - Plug in the USB cable to USB_OTG(P4) and a USB port on the laptop.
  - Click through the add new HW wizard if needed.
  - Open a DOS command prompt to ..\VisualDSP 5.0\Blackfin\Examples\usb\host\windows\hostapp.
  - Type hostapp.exe -l(this is an L and not a one)
  - Loopback should run.  You should see no errors from this or red text.
  - Press any key to exit.
  - Remove the USB cable and plug in the cable that has a USB flash memory stick connected to it.
  - Hold down SW12(PB2) and SW10(PB4) while pressing reset
  - Release the reset button continuing to hold down SW12 and SW10 until LED4 lights up.
  - Make sure LED4 goes out and all the LEDs light back up.  If it is blinking very fast then it failed.  
  - If LED4 does not go out within 30 secs then this test failed.
  - This test reads the product id and vendor id for specific USB flash memory sticks used at
    Analog Devices for testing.  If this test fails, it could be because the memory stick used
    does not match one of the ones that we have used.  To add your USB flash memory stick product id
    and vendor id to the list that is accepted, open USB_test.c and alter the CheckDeviceDescriptor
    function.
 
 FPGA Extender Tests:
  - Power up BF548 EZ-kit.
  - Hold SW11(PB3) push button while pressing reset button.
  - Release the reset button continuing to hold down SW11 for 2 seconds.
  - The FPGA tests should continue to loop through.
 
 AV Extender Tests:
  - Power up BF548 EZ-kit.
  - Hold SW10(PB4) push button while pressing reset button.
  - Release the reset button continuing to hold down SW10 for 2 seconds.
  - The AV tests should continue to loop through.

 Testing a board with a BF549 processor:
 
 	Additional switch settings:
 	SW6	OFF, OFF, ON, ON.
 	SW15	OFF, OFF, ON,  ON.

  - Power up BF549 EZ-kit.
  - Hold SW13(PB1) and SW12(PB2) while pressing reset button.
  - Release the reset button continuing to hold down SW13 and SW12 until LED1 lights up.
  - Wait for LED1 to go out.
  - Press each push button for the push button test.
  - Press each key on the keypad.
  - Once the LEDs are blinking left to right turn the rotary switch clockwise until the LEDs go out 
    making sure they are moving faster and faster as you turn.  Also this test may go really fast 
    and then back to a normal speed.  Keep turning still until they go out.
  - Push the Rotary switch and LEDs should reset to normal speed
  - Turn Rotary switch counter clockwise until they are all on or the next test is started
  - Next 9 boxes should appear on the LCD.  Make sure the top row is red, the middle is green, and the last is yellow.
  - Press each box with your finger and it should turn white when you press it.  
    If the border turns blue that means the border was touched.
  - POST test program goes into auto test mode testing CAN and MXVR that are specific to the ADSP-BF549 processor
    as well as all the tests that are tested on the ADSP-BF548 processor.
