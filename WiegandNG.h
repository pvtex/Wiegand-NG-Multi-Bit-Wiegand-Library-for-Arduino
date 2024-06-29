#ifndef _WIEGAND_NG_H
#define _WIEGAND_NG_H

#if defined(ARDUINO) && ARDUINO >= 100
#include "Arduino.h"
#else
#include "WProgram.h"
#endif

class WiegandNG {

private:
	static void ReadD0();
	static void ReadD1();
	static unsigned long 	_wnglastPulseTime;	// time last bits received
	static unsigned int 	_wngbitCounted;		// number of bits arrived at Interrupt pins
	static unsigned int		_wngbufferSize;		// memory (bytes) allocated for buffer
	unsigned int			_wngbitAllocated;	// wiegand bits required
	unsigned int			_wngpacketGap;		// gap between wiegand packet in millisecond
	static String			_strbuffer;
	
public:
	bool begin(uint8_t pinD0, uint8_t pinD1, unsigned int bits, unsigned int packetGap);
	bool available();
	void clear();
	void pause();
	unsigned int getBitCounted();
	unsigned int getBitAllocated();
	unsigned int getBufferSize();
	unsigned int getPacketGap();
	String getRawData();
	long convert(String str);
	long getCode(bool removeParityBits);
	String getUID(bool removeParityBits, bool wiegandReadHex);
	WiegandNG();
	~WiegandNG();
};

#endif
