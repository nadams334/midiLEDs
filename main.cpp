// cmidiin.cpp
#include <iostream>
#include <vector>
#include <fstream>
#include <cstdlib>
#include <csignal>
#include <cmath>

#include "bcm2835.h"
#include "RtMidi.h"

const char* noteMappingFilename = "config/noteMapping.cfg";
const char* colorMappingFilename = "config/colorMapping.cfg";
const unsigned char noteOnCodeMin = (unsigned char)0x90;
const unsigned char noteOnCodeMax = (unsigned char)0x9F;
const unsigned char noteOffCodeMin = (unsigned char)0x80;
const unsigned char noteOffCodeMax = (unsigned char)0x8F;

int* keyboard; // mapping from note number to LED number
const int numFullOctaves = 5;
const int notesPerOctave = 12;
const int numKeys = numFullOctaves * notesPerOctave + 1;

char* LEDdata; // byte buffer to be sent to WPA102 LED strip
const int numLEDs = 288;
const int bytesPerLED = 4;
const int startFrameSize = 4;
const int endFrameSize = (numLEDs/2)/8 + 1;
uint32_t numBytes = numLEDs * bytesPerLED;

// Color settings for each midi channel, loaded from config file
const int dimnessFactor = 8; // divide MIDI velocity by this amount when calculating brightness
const int numChannels = 16;
const int numNotes = 128;
int* red;
int* green;
int* blue;

RtMidiIn* midiin;


void update()
{
	bcm2835_spi_writenb(LEDdata, numBytes);
}

void set_LED(int channel, int note, int velocity)
{
	int num = keyboard[note];

	if (num < 0 ) return; // note not on piano
	
	int r = red[channel];
	int g = green[channel];
	int b = blue[channel];
	
	if (r < 0)
		r = ( rand() % std::abs(r) ) + 1; // random number between 1 and inputted value
		
	if (g < 0)
		g = ( rand() % std::abs(g) ) + 1; // random number between 1 and inputted value
		
	if (b < 0)
		b = ( rand() % std::abs(b) ) + 1; // random number between 1 and inputted value
	
	LEDdata[startFrameSize+bytesPerLED*(num)] = 0xE0 + (velocity / dimnessFactor + (velocity % dimnessFactor != 0)); // ceiling(velocity/dimnessFactor)
	LEDdata[startFrameSize+bytesPerLED*(num)+1] = b;
	LEDdata[startFrameSize+bytesPerLED*(num)+2] = g;
	LEDdata[startFrameSize+bytesPerLED*(num)+3] = r;
	
	update();
}

void onMidiMessageReceived(double deltatime, std::vector< unsigned char > *message, void *userData)
{
	int code = (int) message->at(0);

	if (code >= noteOnCodeMin && code <= noteOnCodeMax)
	{
		set_LED(code - noteOnCodeMin, message->at(1), message->at(2));
	}	
	else if (code >= noteOffCodeMin && code <= noteOffCodeMax)
	{
		set_LED(code - noteOffCodeMin, message->at(1), 0);
	}
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

// Specify the port to open. -1 means create a virtual port
void init(int port)
{
	
	// Signal handler
	
	signal(SIGINT, signal_handler);
	
	srand(time(NULL));
	
	
	// Load config files
	
	// MIDI note - LED position mapping
	
	keyboard = new int[numNotes];
	std::ifstream noteMappingFile;
	noteMappingFile.open(noteMappingFilename);
	int key = 0;
	int LED = 0;
	
	for (int i = 0; i < numNotes; i++)
	{
		keyboard[i] = -1;
	}
	
	if (noteMappingFile.is_open())
	{
		while ( !noteMappingFile.eof() )
		{
			noteMappingFile >> key;
			noteMappingFile >> LED;
			keyboard[key] = LED;
		}
		noteMappingFile.close();
	}
	
	else
	{
		std::cerr << "Could not open MIDI-LED note mapping configuration file: " << noteMappingFilename << "\nExiting..." << std::endl;
		end(1);
	}
	
	
	// MIDI channel - LED color mapping
	
	red = new int[numChannels];
	green = new int[numChannels];
	blue = new int[numChannels];
	
	std::ifstream colorMappingFile;
	colorMappingFile.open(colorMappingFilename);
	
	int chnl = 0;
	int r = 0;
	int g = 0;
	int b = 0;
	
	for (int i = 0; i < numChannels; i++)
	{
		red[i] = 255;
		green[i] = 255;
		blue[i] = 255;
	}
	
	if (colorMappingFile.is_open())
	{
		while ( !colorMappingFile.eof() )
		{
			colorMappingFile >> chnl;
			colorMappingFile >> r;
			colorMappingFile >> g;
			colorMappingFile >> b;
			if (chnl < 1 || chnl > 16)
			{
				std::cerr << "Error in (" << colorMappingFilename << "): Channel must be between 1 and 16 (was " << chnl << ").\nExiting..." << std::endl;
				end(1);
			}	
			if (r < -255 || r > 255)
			{
				std::cerr << "Error in (" << colorMappingFilename << "): Red must be between -255 and 255 (was " << r << ").\nExiting..." << std::endl;
				end(1);
			}	
			if (g < -255 || g > 255)
			{
				std::cerr << "Error in (" << colorMappingFilename << "): Green must be between -255 and 255 (was " << g << ").\nExiting..." << std::endl;
				end(1);
			}	
			if (b < -255 || b > 255)
			{
				std::cerr << "Error in (" << colorMappingFilename << "): Blue must be between -255 and 255 (was " << b << ").\nExiting..." << std::endl;
				end(1);
			}	
			red[chnl-1] = r;
			green[chnl-1] = g;
			blue[chnl-1] = b;
		}
		colorMappingFile.close();
	}
	
	else
	{
		std::cerr << "Could not open MIDI-LED color mapping configuration file: " << colorMappingFilename << "\nUsing all white." << std::endl;
	}
	
	
	// Initialize WPA102 LED strip data buffer
	
	LEDdata = new char[startFrameSize + numBytes + endFrameSize];

	// Generate start frame
	for (unsigned int i = 0; i < startFrameSize; i++)
	{
		LEDdata[i] = 0x00;
	}
	
	// Generate blank data
	for (unsigned int i = startFrameSize; i < startFrameSize + numBytes; i+=4)
	{
		LEDdata[i] = 0xE0 + 0;
		LEDdata[i+1] = 0x00;
		LEDdata[i+2] = 0x00;
		LEDdata[i+3] = 0x00;
	}
	
	// Generate end frame
	for (unsigned int i = startFrameSize + numBytes; i < startFrameSize + numBytes + endFrameSize; i++)
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
	if (port == -1)
	{
		midiin->openVirtualPort();
	}
	// Open specified port
	else 
	{
		midiin->openPort(port);
	}
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

int main(int argc, char** argv)
{
	int port = -1;
	if (argc > 1)
		port = (int)(argv[2]);
	init(port);
	
	loop();

	return end(0);
}
