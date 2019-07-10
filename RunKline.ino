//Fixed initHonda. if else

#define debug Serial
#define bike Serial1
#define TX_PIN 18
#define t0

#define SUCCESS 0
#define TIMEOUT 1
#define BUSY 2
#define BUFFEROVERFLOW 3

// Flag for ECU connection
byte ECUconnected;
int RPM;

// messages
byte message01[] = {0xFE, 0x04, 0x72, 0x8C}; // 'FE 04 72 8C' wakeup
byte message02[] = {0x72, 0x05, 0x00, 0xF0, 0x99}; //'72 05 00 F0 99' # initialise communications
                                                   // The ECU should respond with: 02 04 00 FA
byte message03[] = {0x72, 0x07, 0x72, 0x11, 0x00, 0x14, 0xF0};  //'72 07 72 11 00 14 F0' # request all of table 11 data 2 6 72 11 0 75
  byte resbuf[10];
  byte resState;

void setup()
{
  debug.begin(115200);
  
}

void loop()
{
  char incomingCommand = 0;
  while (debug.available() > 0)
  {
    incomingCommand = debug.read();
    debug.print("\nCommand: ");
    debug.println(incomingCommand);
  }

  if (incomingCommand == 'i')
  {
    ECUconnected = BUSY;
   //Try to connect with the ECU
   while (ECUconnected == BUSY) {
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

    //-------------

    // Request engine RPM
    bike.write(message03, sizeof(message03));
    bike.flush();
    serialFlush();

    // Receive the response. Receiving this way will block the Arduino.
    resState = BUSY;
    while (resState == BUSY) resState = getResponse(resbuf);
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
      ECUconnected = BUSY;
    }
    //-------------
 }
 }
}

void checkResponse(){
  
  while (bike.available() > 0)
  {
    byte incomingByte = bike.read();
    debug.print(incomingByte, HEX);
    debug.print(' ');
  }
}
void serialFlush(){
  
  while(Serial1.available()) Serial1.read();
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
  bike.write(message01, sizeof(message01)); //Send WakeUp
  delay(200);
  bike.write(message02, sizeof(message02)); // initialise communications
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
		else if (resState == TIMEOUT){
		debug.println("Failed init Honda"); // Return false if diagnostic mode request was denied or failed
		return BUSY; 
		}
	}
	else if (resState == TIMEOUT){
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
      while (bike.available() > 0) {
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
     }
      // Read all bytes, reset timer, index and length
     else {
      t = 0;
      index = 0;
      len = 255;
      delay(55);
      return SUCCESS;
     }
 } 
 else {
    t = 0;
    index = 0;
    len = 255;
    return TIMEOUT;
    //return BUSY; 
 }
}
