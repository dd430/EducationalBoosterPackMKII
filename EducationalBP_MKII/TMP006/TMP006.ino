
/*
 Infrared temp sensor TMP006 example for Educational BoosterPack MK II
 http://boosterpackdepot.info/wiki/index.php?title=Educational_BoosterPack_MK_II
 

Modified 03 Dec 2013
by Dung Dang

This example code is in the public domain.
*/

#include <Tmp006.h>

void setup()
{ // Initalizes the TMP006 for operation and for I2C communication
  TMP006_Init(Eight);  // Takes 8 averaged samples for measurement
}

void loop()
{
  //Prints the temperature 
  Print_Temp(Enable); // Enable: Prints temperature (c) and raw register values
                      // Disable: Prints just the temperature (c)
  
}
