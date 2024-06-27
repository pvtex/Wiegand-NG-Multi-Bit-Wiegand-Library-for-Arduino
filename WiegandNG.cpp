#include <WiegandNG.h>
#if defined(ESP8266)
    #define INTERRUPT_ATTR ICACHE_RAM_ATTR
#elif defined(ESP32)
	#define INTERRUPT_ATTR ICACHE_RAM_ATTR
#else
    #define INTERRUPT_ATTR
#endif

// pcintbranch

unsigned long	WiegandNG::_wnglastPulseTime;	// time last bit pulse received
unsigned int	WiegandNG::_wngbitCounted;		// number of bits arrived at Interrupt pins
unsigned int	WiegandNG::_wngbufferSize;		// memory (bytes) allocated for buffer
unsigned char*	WiegandNG::_wngbuffer;
String 			WiegandNG::_strbuffer;


void WiegandNG::shift_left(unsigned char *ar, int size, int shift)
{
	while (shift--) {								// for each bit to shift ...
		int carry = 0;								// clear the initial carry bit.
		int lastElement = size-1;
		for (int i = 0; i < size; i++) {			// for each element of the array, from low byte to high byte
			if (i!=lastElement) {
				// condition ? valueIfTrue : valueIfFalse
				carry = (ar[i+1] & 0x80) ? 1 : 0;
				ar[i] = carry | (ar[i]<<1);
			}
			else {
				//ar[i] <<=1;
				ar[1] = ar[i] << 1;
			}
		}   
	}
}  

void WiegandNG::clear() {							// reset variables to start new capture
	_wngbitCounted=0;
	_wnglastPulseTime = millis();
	_strbuffer = "";
	//interrupts();									// allow interrupt
}

void WiegandNG::pause() {
	//noInterrupts();									// disable interrupt so that user can process data 
}

unsigned char * WiegandNG::getRawData() {
	return _wngbuffer;									// return pointer of the buffer
}

unsigned int WiegandNG::getPacketGap() {
	return _wngpacketGap;
}

unsigned int WiegandNG::getBitAllocated() {
	return _wngbitAllocated;
}

unsigned int WiegandNG::getBitCounted() {
	return _wngbitCounted;
}

unsigned int WiegandNG::getBufferSize() {
	return _wngbufferSize;
}

bool WiegandNG::available() {
	bool ret=false;
	noInterrupts();
	unsigned long tempLastPulseTime = _wnglastPulseTime;
	interrupts();

	unsigned long sysTick = millis();
	//	if ((sysTick - _wnglastPulseTime) > _packetGap) {	// _packetGap (ms) laps
	if ((sysTick - tempLastPulseTime) > _wngpacketGap) {	// _packetGap (ms) laps
		if(_wngbitCounted>0) {							// bits found, must have data, return true
			if(_wngbitCounted<8) {
#ifdef DEBUG
				Serial.print(_wngbitCounted);
				Serial.print(", ");
				Serial.print(sysTick);
				Serial.print(", ");
				Serial.print(_wnglastPulseTime);
				Serial.print(",");
				Serial.println(tempLastPulseTime);
#endif
			}
			ret=true;
		}
		else
		{
			_wnglastPulseTime = millis();
		}
	}
	return ret;
}

INTERRUPT_ATTR void WiegandNG::ReadD0 () {
	_wngbitCounted = _wngbitCounted + 1;									// increment bit count for Interrupt connected to D0
	//shift_left(_wngbuffer,_wngbufferSize,1);				// shift 0 into buffer
	_wngbuffer[_wngbufferSize-1] |=1;	
	_wnglastPulseTime = millis();						// keep track of time last wiegand bit received
	_strbuffer = _strbuffer + "1";
}

INTERRUPT_ATTR void WiegandNG::ReadD1() {
	_wngbitCounted = _wngbitCounted + 1;									// increment bit count for Interrupt connected to D1
	if (_wngbitCounted > (_wngbufferSize * 8)) {
		_wngbitCounted=0;								// overflowed, 
		_strbuffer = "";
	} else {
		//shift_left(_wngbuffer,_wngbufferSize,1);			// shift 1 into buffer
		_wngbuffer[_wngbufferSize-1] |=1;					// set last bit 1
		_wngbuffer[_wngbufferSize-1] = _wngbuffer[_wngbufferSize] | 1;
		_wnglastPulseTime = millis();					// keep track of time last wiegand bit received
		_strbuffer = _strbuffer + "0";
	}
}

bool WiegandNG::begin(uint8_t pinD0, uint8_t pinD1, unsigned int allocateBits, unsigned int packetGap) {
	if (_wngbuffer != NULL) {
		delete [] _wngbuffer;
	}
	_wngpacketGap = packetGap;
	_wngbitAllocated = allocateBits;
	
	_wngbufferSize=(_wngbitAllocated/8);						// calculate the number of bytes required to store wiegand bits
	if((_wngbitAllocated % 8) >0) _wngbufferSize++;			// add 1 extra byte to cater for bits that are not divisible by 8
	_wngbuffer = new unsigned char [_wngbufferSize];			// allocate memory for buffer
	if(_wngbuffer == NULL) return false;					// not enough memory, return false

	clear();
	
	pinMode(pinD0, INPUT);								// set D0 pin as input
	pinMode(pinD1, INPUT);								// set D1 pin as input
	attachInterrupt(digitalPinToInterrupt(pinD0), ReadD0, FALLING);			// hardware interrupt - high to low pulse
	attachInterrupt(digitalPinToInterrupt(pinD1), ReadD1, FALLING);			// hardware interrupt - high to low pulse
	return true;
}

WiegandNG::WiegandNG() {

}

WiegandNG::~WiegandNG() {
	if (_wngbuffer != NULL) {
		delete [] _wngbuffer;
	}
}

long WiegandNG::convert(const char *str)
{
    long result = 0;

    while(*str)
    {
        result <<= 1;
        result += *str++ == '1' ? 1 : 0;
    }
    return result;
}

long WiegandNG::strconvert(String str)
{
    long result = 0;
	int strlength = str.length();

	for (int i=0; i<strlength; i++) {
		result <<= 1;
		if (str.substring(i,i+1) == "1")
		{
			result += 1;
		} else {
			result += 0;
		}
	}
    return result;
}

long WiegandNG::getCode(bool removeParityBits) {
	long code = 0;
	
	unsigned char *buffer=_wngbuffer;
	unsigned int countedBytes = (_wngbitCounted/8);

  	String tempcode = "";

	if ((_wngbitCounted % 8)>0) countedBytes++;
	// unsigned int bitsUsed = countedBytes * 8;
	
	for (unsigned int i=_wngbufferSize-countedBytes; i< _wngbufferSize;i++) {
		unsigned char bufByte=buffer[i];
		for(int x=0; x<8;x++) {
			if ( (((_wngbufferSize-i) *8)-x) <= _wngbitCounted) {
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
	code = strconvert(_strbuffer);
	
	return code;
}

String WiegandNG::getUID(bool removeParityBits, bool wiegandReadHex) {
	String uidstr = "";

	unsigned char *buffer=_wngbuffer;
	unsigned int countedBytes = (_wngbitCounted/8);

  	String tempcode = "";

	if ((_wngbitCounted % 8)>0) countedBytes++;
	// unsigned int bitsUsed = countedBytes * 8;
	
	
	for (unsigned int i=_wngbufferSize-countedBytes; i< _wngbufferSize;i++) {
		unsigned char bufByte=buffer[i];
		for(int x=0; x<8;x++) {
			if ( (((_wngbufferSize-i) *8)-x) <= _wngbitCounted) {
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
	
	/*
	String strbufferinv = "";
	for (int i=(_strbuffer.length()-1); i>=0; i--){
		strbufferinv = strbufferinv + _strbuffer.substring(i,i+1);
	}
	strbuffer = strbufferinv;
	*/

	if (removeParityBits) {
		tempcode = tempcode.substring(1,-1);
		_strbuffer = _strbuffer.substring(1,-1);
	} else 
	{
		tempcode = tempcode;
		_strbuffer = _strbuffer;
	}

	double temp = 0.0;
	int tempint = 0;
	String uidstrtemp = "";

	/*
	for (int i=0; i < (_wngbitAllocated-2); i+=4) {
		temp = convert((tempcode.substring(i, i+4)).c_str());
		tempint = int(temp);
		uidstrtemp += String(tempint, HEX);
	}
	for (int i=uidstrtemp.length()-2; i >= 0; i-=2) {
		uidstr += uidstrtemp.substring(i, i+2);
	}
	*/
	

	String strtemp = "";
	uidstrtemp = "";
	uidstr = "";
	for (int i=0; i < (_wngbitAllocated-2); i+=4) {
		temp = strconvert(_strbuffer.substring(i, i+4));
		tempint = int(temp);
		uidstrtemp += String(tempint, HEX);
	}
	for (int i=uidstrtemp.length()-2; i >= 0; i-=2) {
		uidstr += uidstrtemp.substring(i, i+2);
	}

	return uidstr;
}


