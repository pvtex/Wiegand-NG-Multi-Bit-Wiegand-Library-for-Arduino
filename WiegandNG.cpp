#include <WiegandNG.h>
#if defined(ESP8266)
    #define INTERRUPT_ATTR ICACHE_RAM_ATTR
#elif defined(ESP32)
	#define INTERRUPT_ATTR ICACHE_RAM_ATTR
#else
    #define INTERRUPT_ATTR
#endif

// pcintbranch

volatile unsigned long	WiegandNG::_lastPulseTime;	// time last bit pulse received
volatile unsigned int	WiegandNG::_bitCounted;		// number of bits arrived at Interrupt pins
volatile unsigned char	*WiegandNG::_buffer;		// buffer for data retention
unsigned int			WiegandNG::_bufferSize;		// memory (bytes) allocated for buffer


void shift_left(volatile unsigned char *ar, int size, int shift)
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
	_bitCounted=0;
	_lastPulseTime = millis();
	memset((unsigned char *)_buffer,0,_bufferSize);
	//interrupts();									// allow interrupt
}

void WiegandNG::pause() {
	//noInterrupts();									// disable interrupt so that user can process data 
}

volatile unsigned char * WiegandNG::getRawData() {
	return _buffer;									// return pointer of the buffer
}

unsigned int WiegandNG::getPacketGap() {
	return _packetGap;
}

unsigned int WiegandNG::getBitAllocated() {
	return _bitAllocated;
}

unsigned int WiegandNG::getBitCounted() {
	return _bitCounted;
}

unsigned int WiegandNG::getBufferSize() {
	return _bufferSize;
}

bool WiegandNG::available() {
	bool ret=false;
	noInterrupts();
	unsigned long tempLastPulseTime = _lastPulseTime;
	interrupts();

	unsigned long sysTick = millis();
	//	if ((sysTick - _lastPulseTime) > _packetGap) {	// _packetGap (ms) laps
	if ((sysTick - tempLastPulseTime) > _packetGap) {	// _packetGap (ms) laps
		if(_bitCounted>0) {							// bits found, must have data, return true
			if(_bitCounted<8) {
#ifdef DEBUG
				Serial.print(_bitCounted);
				Serial.print(", ");
				Serial.print(sysTick);
				Serial.print(", ");
				Serial.print(_lastPulseTime);
				Serial.print(",");
				Serial.println(tempLastPulseTime);
#endif
			}
			ret=true;
		}
		else
		{
			_lastPulseTime = millis();
		}
	}
	return ret;
}

INTERRUPT_ATTR void WiegandNG::ReadD0 () {
	_bitCounted = _bitCounted + 1;									// increment bit count for Interrupt connected to D0
	shift_left(_buffer,_bufferSize,1);				// shift 0 into buffer
	_lastPulseTime = millis();						// keep track of time last wiegand bit received
}

INTERRUPT_ATTR void WiegandNG::ReadD1() {
	_bitCounted = _bitCounted + 1;									// increment bit count for Interrupt connected to D1
	if (_bitCounted > (_bufferSize * 8)) {
		_bitCounted=0;								// overflowed, 
	} else {
		shift_left(_buffer,_bufferSize,1);			// shift 1 into buffer
		//_buffer[_bufferSize-1] |=1;					// set last bit 1
		_buffer[_bufferSize-1] = _buffer[_bufferSize] | 1;
		_lastPulseTime = millis();					// keep track of time last wiegand bit received
	}
}

bool WiegandNG::begin(unsigned int allocateBits, unsigned int packetGap) {
	bool ret;
	// newer versions of Arduino provide pin to interrupt mapping
	ret=begin(2, 3, allocateBits, packetGap);
	return ret;
}

bool WiegandNG::begin(uint8_t pinD0, uint8_t pinD1, unsigned int allocateBits, unsigned int packetGap) {
	if (_buffer != NULL) {
		delete [] _buffer;
	}
	_packetGap = packetGap;
	_bitAllocated = allocateBits;
	
	_bufferSize=(_bitAllocated/8);						// calculate the number of bytes required to store wiegand bits
	if((_bitAllocated % 8) >0) _bufferSize++;			// add 1 extra byte to cater for bits that are not divisible by 8
	_buffer = new unsigned char [_bufferSize];			// allocate memory for buffer
	if(_buffer == NULL) return false;					// not enough memory, return false

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
	if (_buffer != NULL) {
		delete [] _buffer;
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

long WiegandNG::getCode(bool removeParityBits) {
	volatile unsigned char *buffer=_buffer;
	unsigned int countedBytes = (_bitCounted/8);

  	String tempcode = "";
  	long code = 0;

	if ((_bitCounted % 8)>0) countedBytes++;
	// unsigned int bitsUsed = countedBytes * 8;
	
	for (unsigned int i=_bufferSize-countedBytes; i< _bufferSize;i++) {
		unsigned char bufByte=buffer[i];
		for(int x=0; x<8;x++) {
			if ( (((_bufferSize-i) *8)-x) <= _bitCounted) {
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
	return code;
}

String WiegandNG::getUID(bool removeParityBits, bool wiegandReadHex) {
	String uidstr = "";

	volatile unsigned char *buffer=_buffer;
	unsigned int countedBytes = (_bitCounted/8);

  	String tempcode = "";

	if ((_bitCounted % 8)>0) countedBytes++;
	// unsigned int bitsUsed = countedBytes * 8;
	
	for (unsigned int i=_bufferSize-countedBytes; i< _bufferSize;i++) {
		unsigned char bufByte=buffer[i];
		for(int x=0; x<8;x++) {
			if ( (((_bufferSize-i) *8)-x) <= _bitCounted) {
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

	if (removeParityBits) {
		tempcode = tempcode.substring(1,-1);
	} else 
	{
		tempcode = tempcode;
	}

	double temp = 0.0;
	int tempint = 0;
	String uidstrtemp = "";

	for (int i=0; i < (_bitAllocated-2); i+=4) {
		temp = convert((tempcode.substring(i, i+4)).c_str());
		tempint = int(temp);
		uidstrtemp += String(tempint, HEX);
	}
	for (int i=uidstrtemp.length()-2; i >= 0; i-=2) {
		uidstr += uidstrtemp.substring(i, i+2);
	}

	return uidstr;
}


