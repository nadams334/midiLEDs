// cmidiin.cpp
#include <iostream>
#include <vector>
#include <fstream>
#include <cstdlib>
#include <csignal>

#include "bcm2835.h"
#include "RtMidi.h"

const char* mappingFilename = "config/pianoLEDmapping.cfg";
const unsigned char noteOnOffCode = (unsigned char)0x90;

char* LEDdata; // byte buffer to be sent to WPA102 LED strip
int* keyboard; // mapping from note number to LED number
const int lowestOctave = 3;
const int numFullOctaves = 5;
const int notesPerOctave = 12;
const int numKeys = numFullOctaves * notesPerOctave + 1;
const int numLEDs = 288;
const int bytesPerLED = 4;
const int startFrameSize = 4;
const int endFrameSize = (numLEDs/2)/8 + 1;
uint32_t numBytes = numLEDs * bytesPerLED;

RtMidiIn* midiin;


void update()
{
	bcm2835_spi_writenb(LEDdata, numBytes);
	/*for (int i = 0; i < numBytes; i+=4)
	{
		std::cout << i << ": " << (int)LEDdata[i] << std::endl << i+1 << ": " << (int)LEDdata[i+1] << std::endl << i+2 << ": " << (int)LEDdata[i+2] << std::endl << i+3 << ": " << (int)LEDdata[i+3] << std::endl;
		std::cout << std::endl;
	}*/
}

void set_LED(int note, int velocity)
{
	int num = keyboard[note - notesPerOctave*lowestOctave];
	LEDdata[startFrameSize+bytesPerLED*(num)] = 0xE0 + velocity/4;
	LEDdata[startFrameSize+bytesPerLED*(num)+1] = 100 * (velocity > 0);
	LEDdata[startFrameSize+bytesPerLED*(num)+2] = 100 * (velocity > 0);
	LEDdata[startFrameSize+bytesPerLED*(num)+3] = 100 * (velocity > 0);
	update();
}

void onMidiMessageReceived(double deltatime, std::vector< unsigned char > *message, void *userData)
{
	if (message->at(0) == noteOnOffCode)
		set_LED(message->at(1), message->at(2));
}

void clear_LEDs()
{
	for (int i = startFrameSize; i < numLEDs * bytesPerLED + startFrameSize; i+=4)
	{
		LEDdata[i] = 0xE0 + 0;
		LEDdata[i+1] = 0;
		LEDdata[i+2] = 0;
		LEDdata[i+3] = 0;
	}
	update();
}

int end(int status)
{
	clear_LEDs();
	bcm2835_spi_end();
	bcm2835_close();
	delete midiin;
	return status;
}

void signal_handler(int signal)
{
    fprintf(stdout, "\nReceived signal %d. Exiting...\n", signal);
    end(0);
}

void init()
{
	
	// Signal handler
	
	signal(SIGINT, signal_handler);
	
	
	// Initialize mapping from config file
	
	keyboard = new int[numKeys];
	std::ifstream mappingFile;
	mappingFile.open(mappingFilename);
	int key = 0;
	int LED = 0;
	
	if (mappingFile.is_open())
	{
		while ( !mappingFile.eof() )
		{
			mappingFile >> key;
			mappingFile >> LED;
			keyboard[key] = LED;
		}
		mappingFile.close();
	}
	
	else
	{
		std::cerr << "Could not open MIDI-LED mapping configuration file: " << mappingFilename << "\n Exiting...";
		end(1);
	}
	
	
	// Initialize WPA102 LED strip data buffer
	
	LEDdata = new char[startFrameSize + numBytes + endFrameSize];

	// Generate start frame
	for (int i = 0; i < startFrameSize; i++)
	{
		LEDdata[i] = 0x00;
	}
	
	// Generate blank data
	for (int i = startFrameSize; i < startFrameSize + numBytes; i+=4)
	{
		LEDdata[i] = 0xE0 + 0;
		LEDdata[i+1] = 0x00;
		LEDdata[i+2] = 0x00;
		LEDdata[i+3] = 0x00;
	}
	
	// Generate end frame
	for (int i = startFrameSize + numBytes; i < startFrameSize + numBytes + endFrameSize; i++)
	{
		LEDdata[i] = 0x00;
	}

	numBytes += startFrameSize;
	numBytes += endFrameSize;
	
	
	// Initialize bcm2835 for WPA102 LED strip communication
	
	if (!bcm2835_init())
	{
		std::cerr << "bcm2835_init failed. Are you running as root??\n";
	}
	if (!bcm2835_spi_begin())
	{
		std::cerr << "bcm2835_spi_begin failed. Are you running as root??\n";
	}
	bcm2835_spi_setBitOrder(BCM2835_SPI_BIT_ORDER_MSBFIRST);      // The default
	bcm2835_spi_setDataMode(BCM2835_SPI_MODE0);                   // The default
	bcm2835_spi_setClockDivider(BCM2835_SPI_CLOCK_DIVIDER_16);
	bcm2835_spi_chipSelect(BCM2835_SPI_CS0);                      // The default
	bcm2835_spi_setChipSelectPolarity(BCM2835_SPI_CS0, LOW);      // the default
	
	
	// Initalize RtMidi port for MIDI communcation
	
	midiin = new RtMidiIn();
	// Create a port that other audio software can output MIDI data to
	midiin->openVirtualPort();
	// Set our callback function.  This should be done immediately after
	// opening the port to avoid having incoming messages written to the
	// queue.
	midiin->setCallback( &onMidiMessageReceived );
	// ignore sysex, timing, or active sensing messages.
	midiin->ignoreTypes( true, true, true );
	
}

void loop()
{
	char input;
	std::cin.get(input);
}

int main()
{
	init();
	
	loop();

	return end(0);
}
