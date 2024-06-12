#include <WiegandNG.h>

WiegandNG wg;

void PrintBinary(WiegandNG &tempwg) {
	volatile unsigned char *buffer=tempwg.getRawData();
	unsigned int bufferSize = tempwg.getBufferSize();
	unsigned int countedBits = tempwg.getBitCounted();

	unsigned int countedBytes = (countedBits/8);
	if ((countedBits % 8)>0) countedBytes++;
	// unsigned int bitsUsed = countedBytes * 8;
	
	for (unsigned int i=bufferSize-countedBytes; i< bufferSize;i++) {
		unsigned char bufByte=buffer[i];
		for(int x=0; x<8;x++) {
			if ( (((bufferSize-i) *8)-x) <= countedBits) {
				if((bufByte & 0x80)) {
					Serial.print("1");
				}
				else {
					Serial.print("0");
				}
			}
			bufByte<<=1;
		}
	}
	Serial.println();
}

unsigned long long convert(const char *str)
{
    unsigned long long result = 0;

    while(*str)
    {
        result <<= 1;
        result += *str++ == '1' ? 1 : 0;
    }
    return result;
}

long BinaryToInt(WiegandNG &tempwg) {
	volatile unsigned char *buffer=tempwg.getRawData();
	unsigned int bufferSize = tempwg.getBufferSize();
	unsigned int countedBits = tempwg.getBitCounted();

	unsigned int countedBytes = (countedBits/8);

  String tempcode = "";
  long code = 0;

	if ((countedBits % 8)>0) countedBytes++;
	// unsigned int bitsUsed = countedBytes * 8;
	
	for (unsigned int i=bufferSize-countedBytes; i< bufferSize;i++) {
		unsigned char bufByte=buffer[i];
		for(int x=0; x<8;x++) {
			if ( (((bufferSize-i) *8)-x) <= countedBits) {
				if((bufByte & 0x80)) {
					tempcode += "1";
				}
				else {
          tempcode += "0";				
        }
      }
			bufByte<<=1;
		}
	}
  
  code = convert(tempcode.c_str());

	return  code;
}

void setup() {
	Serial.begin(115200);

	// for UNO just use wg.begin(), will default to Pin 2 and Pin 3 connected to D0 and D1 respectively
	// initialize Wiegand ND for 48 bit data, every 8 bits take up 1 byte of Arduino memory
	// as long as there is still memory, user can even capture 1024 bit Wiegand by calling wg.begin(1024)

	unsigned int pinD0 = 14;
	unsigned int pinD1 = 12;
	unsigned int wiegandbits = 58;
	unsigned int packetGap = 15;			// 25 ms between packet
	
	if(!wg.begin(pinD0, pinD1, wiegandbits, packetGap)) {
		Serial.println("Out of memory!");
	}
	Serial.println("Ready...");
}

void loop() {
	if(wg.available()) {
		wg.pause();							// pause Wiegand pin interrupts
		Serial.print("Bits=");
		Serial.println(wg.getBitCounted());	// display the number of bits counted
    	Serial.print("RAW STRING (BIN): ");
    	PrintBinary(wg);
    	Serial.print("RAW STRING (DEC): ");
    	Serial.println(BinaryToInt(wg), DEC);
    	Serial.print("RAW STRING (HEX): ");
    	Serial.println(BinaryToInt(wg), HEX);
		wg.clear();							// compulsory to call clear() to enable interrupts for subsequent data
	}
}
