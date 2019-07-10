/*

Copyright (c) 2019 sabsteef

Permission is hereby granted, free of charge, to any person obtaining a copy of this software
and associated documentation files (the "Software"), to deal in the Software without restriction,
including without limitation the rights to use, copy, modify, merge, publish, distribute,
sublicense, and/or sell copies of the Software, and to permit persons to whom the Software is
furnished to do so, subject to the following conditions:

The above copyright notice and this permission notice shall be included in all copies or
substantial portions of the Software.

THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR IMPLIED, INCLUDING
BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE AND
NONINFRINGEMENT. IN NO EVENT SHALL THE AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM,
DAMAGES OR OTHER LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE SOFTWARE.
*/

// Serial for Debug
#define debug Serial
//Serial to connect to the bike
#define bike Serial1
#define TX_PIN 18
#define t0

//Status Flags
#define SUCCESS 0
#define TIMEOUT 1
#define BUSY 2
#define BUFFEROVERFLOW 3

// Flag for ECU connection
byte ECUconnected;
byte resbuf[10];
byte resState;
int RPM;

//initialise communications messages
byte MessageWakeup[] = {0xFE, 0x04, 0x72, 0x8C}; // wakeup
byte MessageInitialise[] = {0x72, 0x05, 0x00, 0xF0, 0x99}; //initialise communications The ECU should respond with: 02 04 00 FA

//get info from ECU VFR Specific
byte message03[] = {0x72, 0x07, 0x72, 0x60, 0x00, 0x06, 0xAF};
byte message04[] = {0x72, 0x07, 0x72, 0x21, 0x00, 0x06, 0xEE}; 
byte message05[] = {0x72, 0x07, 0x72, 0xD0, 0x00, 0x06, 0x3F};
byte message06[] = {0x72, 0x07, 0x72, 0x16, 0x00, 0x06, 0xF9};
byte message07[] = {0x72, 0x07, 0x72, 0xD1, 0x00, 0x06, 0x3E}; 

// needed to calculate checksum  
byte calculate[] = {0x72, 0x07, 0x72, 0x60, 0x00, 0x06};                                                           

void setup() {
 debug.begin(115200); 
}

void loop(){
 
 char incomingCommand = 0;
  while (debug.available() > 0) {
    incomingCommand = debug.read();
    debug.print("\nCommand: ");
    debug.println(incomingCommand);
  }
   if (incomingCommand == 'c'){
      byte calculate2;
      calculate2 = calc_checksum(calculate, sizeof calculate);
      debug.println(calculate2, HEX); 
   }
     
unsigned long startedWaiting = millis();
  //if ((incomingCommand == 'i') || (ECUconnected = BUSY) )
  
  if (incomingCommand == 'i') {
    ECUconnected = BUSY;
    //Try to connect with the ECU
    while (ECUconnected == BUSY){
     ECUconnected = initHonda();
    }
    while (ECUconnected == SUCCESS) {
      bike.write(message03, sizeof(message03));
      bike.flush();
      serialFlush();
      delay(300);
      checkResponse();
      serialFlush();
      delay(200);
      debug.println(" ");
      //-------------
    /*
    // Request engine RPM
    bike.write(message03, sizeof(message03));
    bike.flush();
    serialFlush();
    // Receive the response. Receiving this way will block the Arduino.
    resState = BUSY;
    while (resState == BUSY) {
      resState = getResponse(resbuf);
    }
    // Successfully received the response message which is stored in resbuf
    if (resState == SUCCESS) {
       // Check if response is positive
         if (resbuf[4] == 0x61) {
            // RPM are calculated by 100*resbuf[6]+resbuf[7]
            RPM = 100 * resbuf[6] + resbuf[7];
            // Do something with RPM e.g. show on LCD, write to file on SD,
         } 
         else {
            // No positive response, reset ECU connection
            ECUconnected = BUSY;
         }
    } 
    else if (resState == TIMEOUT) {
      // Timeout, reinitiate ECU
      */
      
   // }
   
    //-------------
  }
    ECUconnected = BUSY;
 }
}

//function to calculate Checksome
uint8_t calc_checksum(const uint8_t data[], const uint8_t data_len){
  uint8_t cs = 0;
  for (uint8_t i = 0; i < data_len; i++){
    cs += data[i];
  }
  uint8_t CS;
  CS = 0x100 - cs; 
  return CS;
}

// Simple check the response to debug terminal 
void checkResponse(){
  
  while (bike.available() > 0){
    
    byte incomingByte = bike.read();
    debug.print(incomingByte, HEX);
    debug.print(' ');
  }
}

void serialFlush(){
  while(bike.available()) bike.read();
}   

byte initHonda(){
  
  debug.println("Starting init Honda");
  bike.end();
  delay(350);
  //Sending LOW
  digitalWrite(TX_PIN, LOW);
  delay(70);
  //Sending High
  digitalWrite(TX_PIN, HIGH); // this is not an error, it has to came before the pinMode
  delay(120);
  pinMode(TX_PIN, OUTPUT);
  //Start Bike serial1
  bike.begin(10400);
  bike.write(MessageWakeup, sizeof(MessageWakeup)); //Send WakeUp
  delay(200);
  bike.write(MessageInitialise, sizeof(MessageInitialise)); // initialise communications
  bike.flush(); // wait to send all
  
  resState = BUSY;
  // Try to do read a wakeup
  while (resState == BUSY) resState = getResponse(resbuf);
  if (resState == SUCCESS) {
    delay(300);
    resState = BUSY;
    while (resState == BUSY) resState = getResponse(resbuf);
    if (resState == SUCCESS) {
    
       if (resState == SUCCESS) {
          return SUCCESS; 
       } else {
            debug.println("Failed init Honda somthing is wrong retry"); // Return false if diagnostic mode request was denied or failed
            return BUSY; 
         }
    }
    if (resState == TIMEOUT){
        debug.println("Failed init Honda"); // Return false if diagnostic mode request was denied or failed
        return BUSY; 
    }
 }
 if (resState == TIMEOUT){
  debug.println("Failed WakeUp Honda"); // Return false if diagnostic mode request was denied or failed 
  return BUSY; 
 }
}

byte getResponse(byte *rbuffer) {
  // These bytes remain unchanged in between function calls
  // Used to monitor the response while not blocking the program
  static int index = 0;
  static int len = 255;
  static unsigned long t = 0;
 
  // After a request is send the timer is set
  if (t == 0) t = millis();
  // Check if function call is within timeout limit
  if (millis() - t < 250) {
    // Check if more bytes need to be received
    if (index < len) {
      // Check if bytes are available
      unsigned long startedWaiting = millis();
      while ((bike.available() > 0) ||  (millis() - startedWaiting <= 2000)) {
        // Reset timer
        t=0;
        // Read one byte
        //debug.println("Read one byte");
        rbuffer[index] = bike.read();
        //debug.println(rbuffer[0], HEX);
        // First received byte must be correct 0xFE, 0x04, 0x72, 0x8C
        if (rbuffer[0] == 0xFE){
          // Clear bike RX buffer
          while(bike.available()) bike.read();
          debug.println("Wakeup OK");
              t = 0;
              index = 0;
              len = 255; 
          return SUCCESS;
         }

         if ((rbuffer[0] == 0x02) && (rbuffer[1] == 0x04) && (rbuffer[2] == 0x00)){
            // Clear bike RX buffer
            debug.println("clear responce");  
            while(bike.available()) bike.read();
            debug.println("ini OK");  
              t = 0;
              index = 0;
              len = 255;
            return SUCCESS;
           
         }
          index++;
          return BUSY; 
          
       }
       debug.println("Timeout");
        return TIMEOUT;
        
     }
      // Read all bytes, reset timer, index and length
     else {
      t = 0;
      index = 0;
      len = 255;
      delay(55);
      debug.println("succes");
      return SUCCESS;
     }
 } 
 else {
    t = 0;
    index = 0;
    len = 255;
    debug.println("Timeout");
    return TIMEOUT;
    //return BUSY; 
 }
}
