// midiLEDs
// Author: Nathan Adams
// 2016-2017

#include <iostream>
#include <fstream>
#include <unistd.h>
#include <cstdlib>
#include <csignal>
#include <cmath>
#include <string.h>
#include <chrono>
#include <vector>
#include <deque>

#include "bcm2835.h"
#include "RtMidi.h"

typedef void (*funcPtr)(void);

class Command
{

private:

	std::string name;
	funcPtr functionPtr;
	
public:

	Command(std::string nameArg, funcPtr functionPtrArg)
	{
		name = nameArg;
		functionPtr = functionPtrArg;
	}

	std::string getName()
	{
		return name;
	}

	funcPtr getFunctionPointer()
	{
		return functionPtr;
	}
};

class LED_MESSAGE
{
	
private:
	
	int chnl;
	int LED_NUM;
	int brightness;
	int r;
	int g;
	int b;
	
	
public:
	
	LED_MESSAGE(int channel, int LED_number, int brightness_level, int red, int green, int blue)
	{
		chnl = channel;
		LED_NUM = LED_number;
		brightness = brightness_level;
		r = red;
		g = green;
		b = blue;
		
	}
	
	int getChannel()
	{
		return chnl;
	}
	
	int GET_LED_NUM()
	{
		return LED_NUM;
	}
	
	int getBrightness()
	{
		return brightness;
	}
	
	int getRed()
	{
		return r;
	}
	
	int getGreen()
	{
		return g;
	}
	
	int getBlue()
	{
		return b;
	}
	
};


const char* noteMappingFilename = "config/noteMapping.cfg";
const char* colorMappingFilename = "config/colorMapping.cfg";


const unsigned char noteOnCodeMin = (unsigned char)0x90;
const unsigned char noteOnCodeMax = (unsigned char)0x9F;
const unsigned char noteOffCodeMin = (unsigned char)0x80;
const unsigned char noteOffCodeMax = (unsigned char)0x8F;

const unsigned char ccStatusCodeMin = (unsigned char)0xB0;
const unsigned char ccStatusCodeMax = (unsigned char)0xBF;


const long UPDATE_COOLDOWN_MICROSECONDS = 50;
std::chrono::high_resolution_clock::time_point ticks;


int* keyboard; // mapping from note number to LED number
int* noteboard; // reverse mapping
char* LEDdata; // byte buffer to be sent to APA102 LED strip
const int numLEDs = 288;
const int bytesPerLED = 4;
const int startFrameSize = 4;
//const int endFrameSize = (numLEDs/2)/8 + 1;
uint32_t numBytes = numLEDs * bytesPerLED;


const int dimnessFactor = 8; // divide MIDI velocity by this amount when calculating brightness
const int numChannels = 16;
const int numNotes = 128;

bool dynamic_colors = false;
int cc_red = 16;
int cc_green = 17;
int cc_blue = 18;

int* redConfig;
int* greenConfig;
int* blueConfig;

int* red;
int* green;
int* blue;

bool* channelNeedsUpdateMessage;
int cc_update_channel = 30;
int cc_update_all = 31;

int cc_sostenuto = 66;

std::deque<LED_MESSAGE*> activeMessagesForNote[numNotes];
std::vector<LED_MESSAGE*> sostenutoMessages[numChannels];
std::vector<LED_MESSAGE*> pending_LED_messages[numChannels];

std::vector<Command*> commands;


RtMidiIn* midiin;




void set_LED(LED_MESSAGE* message)
{	/*
	std::cout << "Loading the following LED_MESSAGE: " << std::endl;
	std::cout << "Channel: " << message->getChannel() << std::endl;
	std::cout << "LED_NUM: " << message->GET_LED_NUM() << std::endl;
	std::cout << "Brightness: " << message->getBrightness() << std::endl;
	std::cout << "Red: " << message->getRed() << std::endl;
	std::cout << "Green: " << message->getGreen() << std::endl;
	std::cout << "Blue: " << message->getBlue() << std::endl;
	std::cout << std::endl;
	*/
	LEDdata[startFrameSize+bytesPerLED*(message->GET_LED_NUM())] = 0xE0 + message->getBrightness();
	LEDdata[startFrameSize+bytesPerLED*(message->GET_LED_NUM())+1] = message->getBlue();
	LEDdata[startFrameSize+bytesPerLED*(message->GET_LED_NUM())+2] = message->getGreen();
	LEDdata[startFrameSize+bytesPerLED*(message->GET_LED_NUM())+3] = message->getRed();
}

void write_LED_data()
{
	ticks = std::chrono::high_resolution_clock::now();
	bcm2835_spi_writenb(LEDdata, numBytes);
}

void update(int channel)
{
	bool shouldUpdate = true;
	/*
	if (std::chrono::duration_cast<std::chrono::nanoseconds>(std::chrono::high_resolution_clock::now()-ticks).count()/1000 < UPDATE_COOLDOWN_MICROSECONDS)
	{
		shouldUpdate = false;
	}
	*/
	if (shouldUpdate)
	{	
		// set any pending LEDs
		if (channel < 0) // update all
		{
			for (int c = 0; c < numChannels; c++)
			{
				for (unsigned int i = 0; i < pending_LED_messages[c].size(); i++)
				{
					set_LED(pending_LED_messages[c][i]);
				}
				pending_LED_messages[c].clear();
			}
		}
		else if (channel < numChannels) // update just this channel
		{
			if (channelNeedsUpdateMessage[channel])
			{
				for (unsigned int i = 0; i < pending_LED_messages[channel].size(); i++)
				{
					set_LED(pending_LED_messages[channel][i]);
				}
				pending_LED_messages[channel].clear();
			}
		}
		else // error
		{
			std::cerr << "WARNING: Internal error: Attempted to update channel " << channel << ". Skipping update." << std::endl;
		}
		
		write_LED_data();
	}
}

int ceiling(int dividend, int divisor)
{
	return (dividend / divisor) + ((dividend % divisor) != 0);
}

const int endFrameSize = ceiling(numLEDs/2, 8);

void setNote(int channel, int note, int velocity)
{
	int key = keyboard[note];

	if (key < 0 ) return; // note not on piano
	
	int r = red[channel];
	int g = green[channel];
	int b = blue[channel];
	
	int brightness = ceiling(velocity, dimnessFactor);
	
	
	if (r < 0)
		r = ( rand() % std::abs(r) ) + 1; // random number between 1 and inputted value
		
	if (g < 0)
		g = ( rand() % std::abs(g) ) + 1; // random number between 1 and inputted value
		
	if (b < 0)
		b = ( rand() % std::abs(b) ) + 1; // random number between 1 and inputted value
	
	
	LED_MESSAGE* message = new LED_MESSAGE(channel, key, brightness, r, g, b);
	
	if (brightness > 0) // turning note on
	{
		// indicate that this channel is the most recent channel to turn this note on
		activeMessagesForNote[note].push_back(message);
	}
	
	else if (brightness == 0) // turning note off
	{
		// need to check if any other channels still have this note turned on before turning LED off
		if (!activeMessagesForNote[note].empty())
		{
			// Indicate that this note is no longer active for this channel
			for (unsigned int i = 0; i < activeMessagesForNote[note].size(); i++)
			{
				if (activeMessagesForNote[note][i]->getChannel() == channel)
				{
					activeMessagesForNote[note].erase(activeMessagesForNote[note].begin()+i);
					break;
				}
			}
			
			if (!activeMessagesForNote[note].empty() && !channelNeedsUpdateMessage[channel]) // this note is still active for another channel
			{
				// need to set LED back to its previous color
				message = activeMessagesForNote[note].back();
			}
		}
		else // this note is not currently active on any channels
		{
			// continue to disable LED (set to 0 brightness)
		}
	}
	
	else
	{
		std::cerr << "WARNING: Invalid velocity for note " << note << " on channel " << channel << ". Note message ignored." << std::endl;
		return;
	}
	

	set_LED(message);
	
	if (channelNeedsUpdateMessage[channel])
	{
		//pending_LED_messages[channel].push_back(message);
	}
	else // update immediately, do not wait for update message
	{
		//set_LED(message);
		update(channel);
	}
	
}

void onMidiMessageReceived(double deltatime, std::vector< unsigned char > *message, void *userData)
{
	int code = (int) message->at(0);

	if (code >= noteOnCodeMin && code <= noteOnCodeMax)
	{
		setNote(code - noteOnCodeMin, message->at(1), message->at(2));
	}	
	else if (code >= noteOffCodeMin && code <= noteOffCodeMax)
	{
		setNote(code - noteOffCodeMin, message->at(1), 0);
	}
	else if (code >= ccStatusCodeMin && code <= ccStatusCodeMax)
	{
		int ccCode = (int) message->at(1);
		int value = (int) message->at(2);
		int channel = code - (int) ccStatusCodeMin;

		if (ccCode == cc_sostenuto)
		{
			if (value > 0) // turning on
			{
				// need to save all currently active LEDs
				for (int i = 0; i < numNotes; i++)
				{
					unsigned int size = activeMessagesForNote[i].size();
					for (unsigned int j = 0; j < size; j++)
					{
						LED_MESSAGE* ledMessage = activeMessagesForNote[i][j];

						int messageChannel = ledMessage->getChannel();
						if (messageChannel == channel)
						{
							activeMessagesForNote[i].push_back(ledMessage);
							sostenutoMessages[channel].push_back(ledMessage);
						}
					}
				}
				/*
				std::cout << "Sostenuto messages after on: " << std::endl;
				for (unsigned int i = 0; i < sostenutoMessages[channel].size(); i++)
				{
					LED_MESSAGE* message = sostenutoMessages[channel][i];
					std::cout << "Message #" << i << ": " << std::endl;
					std::cout << "Channel: " << message->getChannel() << std::endl;
					std::cout << "LED_NUM: " << message->GET_LED_NUM() << std::endl;
					std::cout << "Brightness: " << message->getBrightness() << std::endl;
					std::cout << "Red: " << message->getRed() << std::endl;
					std::cout << "Green: " << message->getGreen() << std::endl;
					std::cout << "Blue: " << message->getBlue() << std::endl;
					std::cout << std::endl;
				}
				*/
			}
			else // turning off
			{
				// need to clear all saved LEDs from previous on message
				for (unsigned int i = 0; i < sostenutoMessages[channel].size(); i++)
				{
					LED_MESSAGE* ledMessage = sostenutoMessages[channel][i];
					setNote(ledMessage->getChannel(), noteboard[ledMessage->GET_LED_NUM()], 0); // send note off message
				}
				sostenutoMessages[channel].clear();
			}
		}
		else if (ccCode == cc_update_channel)
		{
			update(channel);
		}
		
		else if (ccCode == cc_update_all)
		{
			update(-1);
		}

		else if (dynamic_colors)
		{
			if (ccCode == cc_red)
				red[channel] = value;

			else if (ccCode == cc_green)
				green[channel] = value;

			else if (ccCode == cc_blue)
				blue[channel] = value;

			std::cout << "Channel " << channel << " set to color RGB=" << red[channel] << "." << green[channel] << "." << blue[channel] << std::endl;
		}
	}
}

void clear_LEDs()
{
	for (int i = 0; i < numLEDs; i++)
	{
		set_LED(new LED_MESSAGE(0, i, 0, 0, 0, 0));
	}

	for (int i = 0; i < numNotes; i++)
	{
		activeMessagesForNote[i].clear();
	}

	for(int i = 0; i < numChannels; i++)
	{
		sostenutoMessages[i].clear();
	}

	write_LED_data();

	std::cout << "LEDs cleared." << std::endl;
}

void resetColors()
{
	for (int channel = 0; channel < numChannels; channel++)
	{
		red[channel] = redConfig[channel];
		green[channel] = greenConfig[channel];
		blue[channel] = blueConfig[channel];
	}

	std::cout << "All channels reset to default color configuration." << std::endl;
}

void toggleDynamicColors()
{
	if (dynamic_colors)
	{
		dynamic_colors = false;
		std::cout << "Color parameters disabled." << std::endl;
		resetColors();
	}
	else
	{
		dynamic_colors = true;
		std::cout << "Color parameters enabled." << std::endl;
	}
}

void end(int status)
{
	//clear_LEDs();
	bcm2835_spi_end();
	bcm2835_close();
	delete midiin;
	exit(status);
}

void end()
{
	end(0);
}

void signal_handler(int signal)
{
	fprintf(stdout, "\nReceived signal %d. Send EOF (Ctrl+D) to quit.\n", signal);
	std::cout << "Enter (code) to issue command:" << std::endl;
	for (unsigned int i = 0; i < commands.size(); i++)
	{
		fprintf(stdout, "(%d) %s\n", i+1, commands[i]->getName().c_str());
	}
	std::cout << std::endl; 
}

void createCommandList()
{
	commands.push_back(new Command("Clear LEDs", &clear_LEDs));
	commands.push_back(new Command("Enable/Disable Color Parameters", &toggleDynamicColors));
}

void init()
{
	ticks = std::chrono::high_resolution_clock::now();

	// Signal handler
	
	signal(SIGINT, signal_handler);
	
	srand(time(NULL));

	createCommandList();
	
	
	// Load config files
	
	// MIDI note - LED position mapping
	
	keyboard = new int[numNotes];
	noteboard = new int[numLEDs];
	std::ifstream noteMappingFile;
	noteMappingFile.open(noteMappingFilename);
	int key = 0;
	int LED = 0;
	
	for (int i = 0; i < numNotes; i++)
	{
		keyboard[i] = -1;
	}

	for (int i = 0; i < numLEDs; i++)
	{
		noteboard[i] = -1;
	}
	
	if (noteMappingFile.is_open())
	{
		while ( !noteMappingFile.eof() )
		{
			noteMappingFile >> key;
			noteMappingFile >> LED;
			keyboard[key] = LED;
			noteboard[LED] = key;
		}
		noteMappingFile.close();
	}
	
	else
	{
		std::cerr << "Could not open MIDI-LED note mapping configuration file: " << noteMappingFilename << "\nExiting..." << std::endl;
		end(1);
	}
	/*
	for (int i = 0; i < numNotes; i++)
	{
		std::cout << "keyboard[" << i << "]: " << keyboard[i] << std::endl;
	}

	std::cout << std::endl;

	for (int i = 0; i < numLEDs; i++)
	{
		std::cout << "noteboard[" << i << "]: " << noteboard[i] << std::endl;
	}
	*/
	
	// MIDI channel - LED color mapping

	redConfig = new int[numChannels];
	greenConfig = new int[numChannels];
	blueConfig = new int[numChannels];

	red = new int[numChannels];
	green = new int[numChannels];
	blue = new int[numChannels];
	channelNeedsUpdateMessage = new bool[numChannels];
	
	std::ifstream colorMappingFile;
	colorMappingFile.open(colorMappingFilename);
	
	int chnl = 0;
	int r = 0;
	int g = 0;
	int b = 0;
	
	for (int i = 0; i < numChannels; i++)
	{
		redConfig[i] = 1;
		greenConfig[i] = 1;
		blueConfig[i] = 1;
		red[i] = redConfig[i];
		green[i] = greenConfig[i];
		blue[i] = blueConfig[i];
		channelNeedsUpdateMessage[i] = false;
	}
	
	if (colorMappingFile.is_open())
	{
		while ( !colorMappingFile.eof() )
		{
			// Static channel colors

			colorMappingFile >> chnl;
			colorMappingFile >> r;
			colorMappingFile >> g;
			colorMappingFile >> b;

			bool chnlNeedsUpdateMessage = false;

			if (chnl < 0)
			{
				chnl = 0 - chnl; // convert to positive equivalent
				chnlNeedsUpdateMessage = true;
			}

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
			redConfig[chnl-1] = r;
			greenConfig[chnl-1] = g;
			blueConfig[chnl-1] = b;

			red[chnl-1] = redConfig[chnl-1];
			green[chnl-1] = greenConfig[chnl-1];
			blue[chnl-1] = blueConfig[chnl-1];
			
			if (chnlNeedsUpdateMessage)
				channelNeedsUpdateMessage[chnl-1] = true;
		}
		colorMappingFile.close();
	}
	
	else
	{
		std::cerr << "Could not open MIDI-LED color mapping configuration file: " << colorMappingFilename << "\nUsing all white." << std::endl;
	}
	
	
	// Initialize APA102 LED strip data buffer
	
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
		LEDdata[i] = 0xFF;
	}

	numBytes += startFrameSize;
	numBytes += endFrameSize;
	
	
	// Initialize bcm2835 for APA102 LED strip communication
	
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

	fprintf(stdout, "\nReady to interpret MIDI input data through the RtMidi Input Client.\n");
}

void loop()
{
	char input;
	while (std::cin.get(input))
	{
		if (input >= '1' && input <= '9')
		{
			// Execute command
			int commandIndex = input - '1';
			Command* command = commands[commandIndex];
			funcPtr commandFunctionPointer = command->getFunctionPointer();
			std::cout << "Executing command: " << command->getName() << std::endl;
			commandFunctionPointer();
		}
	}
	std::cout << std::endl << "Received EOF." << std::endl;
}

int main(int argc, char** argv)
{
	init();
	
	loop();

	std::cout << std::endl;

	end(0);
}
