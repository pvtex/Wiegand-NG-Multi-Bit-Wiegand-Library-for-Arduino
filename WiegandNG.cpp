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
String 			WiegandNG::_strbuffer;


void WiegandNG::clear() {							// reset variables to start new capture
	_wngbitCounted=0;
	_wnglastPulseTime = millis();
	_strbuffer = "";
	//interrupts();									// allow interrupt
}

void WiegandNG::pause() {
	//noInterrupts();									// disable interrupt so that user can process data 
}
String WiegandNG::getRawData() {
	return _strbuffer;									// return pointer of the buffer
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
	_wnglastPulseTime = millis();						// keep track of time last wiegand bit received
	_strbuffer = _strbuffer + "1";
}

INTERRUPT_ATTR void WiegandNG::ReadD1() {
	_wngbitCounted = _wngbitCounted + 1;									// increment bit count for Interrupt connected to D1
	if (_wngbitCounted > (_wngbufferSize * 8)) {
		_wngbitCounted=0;								// overflowed, 
		_strbuffer = "";
	} else {
		_wnglastPulseTime = millis();					// keep track of time last wiegand bit received
		_strbuffer = _strbuffer + "0";
	}
}

bool WiegandNG::begin(uint8_t pinD0, uint8_t pinD1, unsigned int allocateBits, unsigned int packetGap) {
	if (_strbuffer != "") {
		_strbuffer = "";
	}
	_wngpacketGap = packetGap;
	_wngbitAllocated = allocateBits;
	
	_wngbufferSize=(_wngbitAllocated/8);						// calculate the number of bytes required to store wiegand bits
	if((_wngbitAllocated % 8) >0) _wngbufferSize++;			// add 1 extra byte to cater for bits that are not divisible by 8

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
	if (_strbuffer != "") {
		_strbuffer = "";
	}
}

long WiegandNG::convert(String str)
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
	
	unsigned int countedBytes = (_wngbitCounted/8);

  	String tempcode = "";

	if ((_wngbitCounted % 8)>0) countedBytes++;
	code = convert(_strbuffer);
	
	return code;
}

String WiegandNG::getUID(bool removeParityBits, bool wiegandReadHex) {
	String uidstr = "";

	unsigned int countedBytes = (_wngbitCounted/8);

  	String tempcode = "";

	if ((_wngbitCounted % 8)>0) countedBytes++;

	if (removeParityBits) {
		_strbuffer = _strbuffer.substring(1,-1);
	} else 
	{
		_strbuffer = _strbuffer;
	}

	double temp = 0.0;
	int tempint = 0;
	String uidstrtemp = "";

	String strtemp = "";
	uidstrtemp = "";
	uidstr = "";
	for (int i=0; i < (_wngbitAllocated-2); i+=4) {
		temp = convert(_strbuffer.substring(i, i+4));
		tempint = int(temp);
		uidstrtemp += String(tempint, HEX);
	}
	for (int i=uidstrtemp.length()-2; i >= 0; i-=2) {
		uidstr += uidstrtemp.substring(i, i+2);
	}

	return uidstr;
}


