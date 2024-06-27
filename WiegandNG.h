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
	static void shift_left(unsigned char *ar, int size, int shift);
	static unsigned long 	_wnglastPulseTime;	// time last bits received
	static unsigned int 	_wngbitCounted;		// number of bits arrived at Interrupt pins
	static unsigned int		_wngbufferSize;		// memory (bytes) allocated for buffer
	unsigned int			_wngbitAllocated;	// wiegand bits required
	unsigned int			_wngpacketGap;		// gap between wiegand packet in millisecond
	static unsigned char *	_wngbuffer;			// buffer for data retention
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
	unsigned char *getRawData();
	long convert(const char *str);
	long strconvert(String str);
	long getCode(bool removeParityBits);
	String getUID(bool removeParityBits, bool wiegandReadHex);
	WiegandNG();
	~WiegandNG();
};

#endif
