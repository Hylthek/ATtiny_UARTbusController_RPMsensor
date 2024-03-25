#include <SoftwareSerial.h>
//CUART "header file" (CUART stands for Calvin's UART hehe)===========================================================================================
//CUART changeable variables
const int ID = 1; //can be 1-10, device CANNOT have the same ID as another device on the bus
const int MAX_ARRAY_SIZE = 2; //change if you need to send more than 10 variables (unlikely)

//CUART variables (dont touch)
const int pinTransmitFlag = 0; //bool output pin (LOW = read, HIGH = transmit) (also LED pin)
const int pinTxD = 1; //serial output pin
const int pinRxD = 2; //serial input pin
const int pinSensor1 = 3;
const int pinSensor2 = 4;
SoftwareSerial mySerial(pinRxD, pinTxD, false);
const char EOT = 0; //"EndOfTransmission"
//CUART dataSend (dont touch)
void dataSend(const unsigned long valueArray[MAX_ARRAY_SIZE]) {
  // FORMAT: teensy (master device)        : <myID>------------...-------------<myID+1>-----<myID+2>-ect.
  // FORMAT: ATtiny (child device aka. me!): -------<data><data>...<data><EOT>-----------------------ect.
  
  //wait for correct ID byte
  while (true) {
    while (mySerial.available() > 0)  //clear data queue
      mySerial.read();
    while (mySerial.peek() == -1) {}  //wait for ID byte
    char IdByte = mySerial.read();    //read ID byte
    if (IdByte == ID) {               //exit loop if ID is ours
      break;
    }
  }
  
  //send out data
  digitalWrite(pinTransmitFlag, HIGH);
  delayMicroseconds(100); //100us delay allows bus driver to enable bus driving in time for the start bit
  for (int i = 0; i < MAX_ARRAY_SIZE; i++) {
    mySerial.print(valueArray[i], DEC);  //sends the data out as ascii numbers (ie. '1' '2' '3')
    if(i < MAX_ARRAY_SIZE-1) {
      mySerial.write(',');
    }
  }
  mySerial.write(EOT); //notifies end of the transmission
  digitalWrite(pinTransmitFlag, LOW);
  return;
}
//CUARTsetup (dont touch)
void CUARTsetup() { // put your setup code here, to run once:
  pinMode (pinRxD, INPUT_PULLUP);
  pinMode (pinTxD, OUTPUT);
  pinMode (pinTransmitFlag, OUTPUT);
  digitalWrite(pinTransmitFlag, LOW);

  mySerial.begin(9600);
  // ^^(baud rate)^^ for this number to be accurate, ATtiny MUST be set at "8 MHz(internal)" clock speed
  mySerial.listen(); //marks this serial's reciever as the active one in the chip
}

//end of CUART header, start of ATtiny85 code==============================================================================================================================
  //note: to program a brand-new ATtiny85, you must set "Clock:" to "8 MHz (internal)" and then press "Burn Bootloader"
  //IMPORTANT NOTE: DO NOT BURN BOOTLOADER WHILE CLOCK IS SET TO "<#>MHz (external)" THIS WILL BRICK THE CHIP
  //note: remember to set "ID" and "MAX_ARRAY_SIZE" correctly
  //note: remember to run "CUARTsetup" during "setup"

//setup function
void setup() {
  CUARTsetup(); //<-- dont touch pls
  pinMode (pinSensor1, INPUT_PULLUP);
}

//global vars
const int stuffingCutoff = 1000; //cutoff time until code starts stuffing zeros (mSec)
const int nodesPerRotation = 12; //the number of times the hall effect sensor triggers in one revolution
bool prevSensorState = 0; //previous state of the rpm sensor
unsigned long prevMilliseconds = 0; //time since last sample (mSec)
unsigned long prevMicroseconds = 0; //time since last sample (uSec)
bool newDataRun = true; //makes sure to capture at least 2 edges before sending data

//loop function
void loop() { // put your main code here, to run repeatedly:
  //set "curr" variables---------------------------------------------------------------------------------------
  unsigned long currMilliseconds = millis();
  unsigned long currMicroseconds = micros();
  bool currSensorState = digitalRead(pinSensor1);
  //calculate RPM----------------------------------------------------------------------------------------------
  if (currSensorState != prevSensorState) {
    prevSensorState = digitalRead(pinSensor1);
    if(currSensorState == LOW) {
      //calculate- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
      unsigned long x = currMicroseconds - prevMicroseconds;
      unsigned long RPM = (60000000 / (x * nodesPerRotation)); // RpM = 60,000,000/x/NpR
      //create send array- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
      if(newDataRun == false) {
        unsigned long RPM_Array[MAX_ARRAY_SIZE];
        RPM_Array[0] = currMilliseconds; //total milliseconds
        RPM_Array[1] = RPM;
        dataSend(RPM_Array);
        newDataRun = true;
      }
      else {
        newDataRun = false;
      }
      //update "prev" variables - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
      prevMilliseconds = millis();
      prevMicroseconds = micros();
      return;
    }
  }
  //zero stuffing----------------------------------------------------------------------------------------------
  else if (currMilliseconds - prevMilliseconds > stuffingCutoff) {
    //create send data array- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
    unsigned long dataToSend[MAX_ARRAY_SIZE];
    dataToSend[0] = currMilliseconds;
    dataToSend[1] = 0;
    dataSend(dataToSend);
    //update prevMilliseconds - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
    prevMilliseconds = millis();
    return;
  }
}