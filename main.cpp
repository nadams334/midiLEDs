// midiLEDs
// Author: Nathan Adams
// 2016-2018

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


// Config file loading
const char* noteMappingFilename = "config/noteMapping.cfg";
const char* channelMappingFilename = "config/channelMapping.cfg";


// Midi Messages
const unsigned char noteOnCodeMin = (unsigned char)0x90;
const unsigned char noteOnCodeMax = (unsigned char)0x9F;
const unsigned char noteOffCodeMin = (unsigned char)0x80;
const unsigned char noteOffCodeMax = (unsigned char)0x8F;

const unsigned char ccStatusCodeMin = (unsigned char)0xB0;
const unsigned char ccStatusCodeMax = (unsigned char)0xBF;


const long UPDATE_COOLDOWN_MICROSECONDS = 50;
std::chrono::high_resolution_clock::time_point ticks;


// Keyboard config
int lowestNoteOnPiano;
int highestNoteOnPiano;
int* keyboard; // mapping from note number to LED number
int* noteboard; // reverse mapping


// LED counts
char* LEDdata; // byte buffer to be sent to APA102 LED strip
const int numLEDs = 288;
const int bytesPerLED = 4;
const int startFrameSize = 4;
//const int endFrameSize = (numLEDs/2)/8 + 1;
uint32_t numBytes = numLEDs * bytesPerLED;


const int dimnessFactor = 8; // divide MIDI velocity by this amount when calculating brightness
const int numChannels = 16;
const int numNotes = 128;

bool dataDump = false;

bool dynamic_colors = true;
int cc_red = 16;
int cc_green = 17;
int cc_blue = 18;

const int MAX_VELOCITY = 127;
const int MAX_LED_VALUE = 255;

int mostRecentColorMessageChannel = -1;
std::vector<unsigned char>* lastMidiMessageReceived;

int* redConfig;
int* greenConfig;
int* blueConfig;

int* red;
int* green;
int* blue;

bool* channelIsLowPriority;
bool* channelNeedsUpdateMessage;
int cc_update_channel = 30;
int cc_update_all = 31;

bool* damperActive;
int cc_damper = 64;
bool* sostenutoActive;
int cc_sostenuto = 66;

std::deque<LED_MESSAGE*> activeMessagesForNote[numNotes];
std::vector<LED_MESSAGE*> sostenutoMessages[numChannels];
std::vector<LED_MESSAGE*> damperMessages[numChannels];
//std::vector<LED_MESSAGE*> pending_LED_messages[numChannels];

std::vector<Command*> commands;


RtMidiIn* midiIn;
const std::string DEFAULT_RTMIDI_NAME = "midiLEDs-input";

bool autoConnectALSAPorts = true; // use aconnect to automatically establish ALSA connection on launch
const std::string DEFAULT_ALSA_INPUT_NAME = "CH345";
const std::string DEFAULT_ALSA_OUTPUT_NAME = DEFAULT_RTMIDI_NAME;



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
	/*
	bool shouldUpdate = true;
	
	if (std::chrono::duration_cast<std::chrono::nanoseconds>(std::chrono::high_resolution_clock::now()-ticks).count()/1000 < UPDATE_COOLDOWN_MICROSECONDS)
	{
		shouldUpdate = false;
	}
	
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

		if (dataDump)	return;
		
		write_LED_data();
	}
	*/

	if (dataDump)	return;
		
	write_LED_data();
}

int ceiling(int dividend, int divisor)
{
	return (dividend / divisor) + ((dividend % divisor) != 0);
}

const int endFrameSize = ceiling(numLEDs/2, 8);

void setNote(int channel, int note, int velocity)
{
	if (note < lowestNoteOnPiano)
	{
		if (channel % 2 == 0)
		{
			note = 0; // C0, first natural note, should be set to the bottom left indicator
		}
		else
		{
			note = 1; // C#0, first accidental note, should be set to the top left indicator
		}
	}
	else if (note > highestNoteOnPiano)
	{
		if (channel % 2 == 0)
		{
			note = 127; // G10, last natural note, should be set to bottom right indicator
		}
		else
		{
			note = 126; // F#10, last accidental note, should be set to top right indicator
		}
	}

	int key = keyboard[note];
	
	int r = red[channel];
	int g = green[channel];
	int b = blue[channel];

	if (r == 0 && g == 0 && b == 0) return; // ignore notes on this channel
	
	int brightness = ceiling(velocity, dimnessFactor);
	
	
	if (r < 0)
		r = ( rand() % std::abs(r) ) + 1; // random number between 1 and inputted value
		
	if (g < 0)
		g = ( rand() % std::abs(g) ) + 1; // random number between 1 and inputted value
		
	if (b < 0)
		b = ( rand() % std::abs(b) ) + 1; // random number between 1 and inputted value
	
	// modify color intensities based on note velocity
	r *= ceiling(velocity, ceiling(MAX_VELOCITY, 2));
	g *= ceiling(velocity, ceiling(MAX_VELOCITY, 2));
	b *= ceiling(velocity, ceiling(MAX_VELOCITY, 2));
	
	LED_MESSAGE* message = new LED_MESSAGE(channel, key, brightness, r, g, b);
	
	if (brightness > 0) // turning note on
	{
		if (channelIsLowPriority[channel])
		{
			activeMessagesForNote[note].push_back(message);
			if (damperActive[channel])
			{
				activeMessagesForNote[note].push_back(message);
				damperMessages[channel].push_back(message);
			}

			message = activeMessagesForNote[note].front();
		}

		else
		{
			activeMessagesForNote[note].push_front(message);
			if (damperActive[channel])
			{
				activeMessagesForNote[note].push_front(message);
				damperMessages[channel].push_back(message);
			}
		}
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
			
			if (!activeMessagesForNote[note].empty()) // this note is still active for another channel
			{
				// need to set LED back to its previous color
				message = activeMessagesForNote[note].front();
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

void handleDamperMessage(bool enable, int channel)
{
	if (enable)
	{
		if (damperActive[channel])
		{
			//std::cerr << "WARNING: Received damper ON request, but damper is already active. Ignoring request." << std::endl;
			return;
		}

		damperActive[channel] = true;

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
					activeMessagesForNote[i].push_front(ledMessage);
					damperMessages[channel].push_back(ledMessage);
				}
			}
		}
	}
	else
	{
		if (!damperActive[channel])
		{
			//std::cerr << "WARNING: Received damper OFF request, but damper is not currently active. Ignoring request." << std::endl;
			return;
		}

		damperActive[channel] = false;

		// need to clear all saved LEDs from previous on message
		for (unsigned int i = 0; i < damperMessages[channel].size(); i++)
		{
			LED_MESSAGE* ledMessage = damperMessages[channel][i];
			setNote(ledMessage->getChannel(), noteboard[ledMessage->GET_LED_NUM()], 0); // send note off message
		}
		damperMessages[channel].clear();
	}
}

void handleSostenutoMessage(bool enable, int channel)
{
	if (enable)
	{
		if (sostenutoActive[channel])
		{
			//std::cerr << "WARNING: Received sostenuto ON request, but sostenuto is already active. Ignoring request." << std::endl;
			return;
		}

		sostenutoActive[channel] = true;

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
					activeMessagesForNote[i].push_front(ledMessage);
					sostenutoMessages[channel].push_back(ledMessage);
				}
			}
		}
	}
	else
	{
		if (!sostenutoActive[channel])
		{
			//std::cerr << "WARNING: Received sostenuto OFF request, but sostenuto is not currently active. Ignoring request." << std::endl;
			return;
		}

		sostenutoActive[channel] = false;

		// need to clear all saved LEDs from previous on message
		for (unsigned int i = 0; i < sostenutoMessages[channel].size(); i++)
		{
			LED_MESSAGE* ledMessage = sostenutoMessages[channel][i];
			setNote(ledMessage->getChannel(), noteboard[ledMessage->GET_LED_NUM()], 0); // send note off message
		}
		sostenutoMessages[channel].clear();
	}
}

void dumpDataStructures()
{
	for (int note = 0; note < numNotes; note++)
	{
		for (unsigned int messageIndex = 0; messageIndex < activeMessagesForNote[note].size(); messageIndex++)
		{
			LED_MESSAGE* message = activeMessagesForNote[note][messageIndex];
			fprintf(stdout, "activeMessagesForNote[%d][%d]: Channel=%d, LED=%d, Brightness=%d, RGB=%03d.%03d.%03d\n", note, messageIndex, message->getChannel(), message->GET_LED_NUM(), message->getBrightness(), message->getRed(), message->getGreen(), message->getBlue());
		}
	}

	std::cout << std::endl;
	
	for (int i = 0; i < numChannels; i++)
	{
		std::cout << "Sostenuto[" << i << "] active: " << sostenutoActive[i] << std::endl;
	}

	for (int channel = 0; channel < numChannels; channel++)
	{
		for (unsigned int messageIndex = 0; messageIndex < sostenutoMessages[channel].size(); messageIndex++)
		{
			LED_MESSAGE* message = sostenutoMessages[channel][messageIndex];
			fprintf(stdout, "sostenutoMessages[%d][%d]: Channel=%d, LED=%d, Brightness=%d, RGB=%03d.%03d.%03d\n", channel, messageIndex, message->getChannel(), message->GET_LED_NUM(), message->getBrightness(), message->getRed(), message->getGreen(), message->getBlue());
		}
	}

	std::cout << std::endl;

	for (int i = 0; i < numChannels; i++)
	{
		std::cout << "Damper[" << i << "] active: " << damperActive[i] << std::endl;
	}

	for (int channel = 0; channel < numChannels; channel++)
	{
		for (unsigned int messageIndex = 0; messageIndex < damperMessages[channel].size(); messageIndex++)
		{
			LED_MESSAGE* message = damperMessages[channel][messageIndex];
			fprintf(stdout, "damperMessages[%d][%d]: Channel=%d, LED=%d, Brightness=%d, RGB=%03d.%03d.%03d\n", channel, messageIndex, message->getChannel(), message->GET_LED_NUM(), message->getBrightness(), message->getRed(), message->getGreen(), message->getBlue());
		}
	}
}

void dumpMidiMessage(std::vector<unsigned char>* message)
{
	for (unsigned int i = 0; i < message->size(); i++)
	{
		fprintf(stdout, "%02X ", message->at(i));
	}
	std::cout << std::endl;
}

void onMidiMessageReceived(double deltatime, std::vector<unsigned char>* message, void* userData)
{
	lastMidiMessageReceived->clear();
	for (unsigned int i = 0; i < message->size(); i++)
	{
		lastMidiMessageReceived->push_back(message->at(i));
	}

	if (dataDump)
	{
		dumpMidiMessage(message);
	}

	// Determine if the message's Status Byte indicates a Note Message or a Control Change Message

	int code = (int) message->at(0);

	// Note On message
	if (code >= noteOnCodeMin && code <= noteOnCodeMax)
	{
		if (dynamic_colors && (message->at(1) == cc_red || message->at(1) == cc_green || message->at(1) == cc_blue))
			return; // Ignore Note On messages that look like Color Change messages
		int channel = code - noteOnCodeMin;
		setNote(channel, message->at(1), message->at(2));
	}
	// Note Off message
	else if (code >= noteOffCodeMin && code <= noteOffCodeMax)
	{
		int channel = code - noteOffCodeMin;
		setNote(channel, message->at(1), 0);
	}
	// Control Change message
	else if (code >= ccStatusCodeMin && code <= ccStatusCodeMax)
	{
		int ccCode = (int) message->at(1);
		int value = (int) message->at(2);
		int channel = code - (int) ccStatusCodeMin;

		if (ccCode == cc_sostenuto)
		{
			handleSostenutoMessage(value > 0, channel);
		}
		else if (ccCode == cc_damper)
		{
			handleDamperMessage(value > 0, channel);
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
			{
				mostRecentColorMessageChannel = channel;

				red[channel] = value;

				if (redConfig[channel] < 0)
				{
					red[channel] = 0 - std::abs(red[channel]); // force negative
				}
			}
			else if (ccCode == cc_green)
			{
				mostRecentColorMessageChannel = channel;

				green[channel] = value;

				if (greenConfig[channel] < 0)
				{
					green[channel] = 0 - std::abs(green[channel]); // force negative
				}
			}
			else if (ccCode == cc_blue)
			{
				mostRecentColorMessageChannel = channel;

				blue[channel] = value;

				if (blueConfig[channel] < 0)
				{
					blue[channel] = 0 - std::abs(blue[channel]); // force negative
				}
			}
		}
	}
}

void clearDataStructures()
{
	for (int i = 0; i < numNotes; i++)
	{
		activeMessagesForNote[i].clear();
	}

	for(int i = 0; i < numChannels; i++)
	{
		sostenutoActive[i] = false;
		sostenutoMessages[i].clear();
	}

	for(int i = 0; i < numChannels; i++)
	{
		damperActive[i] = false;
		damperMessages[i].clear();
	}
}

void clear_LEDs()
{
	for (int i = 0; i < numLEDs; i++)
	{
		set_LED(new LED_MESSAGE(0, i, 0, 0, 0, 0));
	}

	clearDataStructures();

	write_LED_data();

	std::cout << "Data cleared." << std::endl;
}

void displayColorMessageCodes()
{
	std::cout << "Send MIDI Control Change #" << cc_red << " to adjust the channel's red value." << std::endl;
	std::cout << "Send MIDI Control Change #" << cc_green << " to adjust the channel's green value." << std::endl;
	std::cout << "Send MIDI Control Change #" << cc_blue << " to adjust the channel's blue value." << std::endl;
}

void toggleDataDump()
{
	if (dataDump)
	{
		dataDump = false;
		std::cout << "Incoming MIDI data is no longer written to standard out." << std::endl;
		std::cout << "Incoming MIDI data is now relayed to LEDs." << std::endl;
	}
	else
	{
		dataDump = true;
		std::cout << "Incoming MIDI data is now written to standard out." << std::endl;
		std::cout << "Incoming MIDI data is no longer relayed to LEDs." << std::endl;
		std::cout << "Last MIDI message received was: ";
		dumpMidiMessage(lastMidiMessageReceived);
	}
}

void toggleDynamicColors()
{
	if (dynamic_colors)
	{
		dynamic_colors = false;
		std::cout << "Color messages disabled." << std::endl;
	}
	else
	{
		dynamic_colors = true;
		std::cout << "Color messages enabled." << std::endl;
	}
}

void displayColorValues(int channel)
{
	if (channel >= 0 && channel < numChannels)
	{
		fprintf(stdout, "Current color for channel %d is RGB=%03d.%03d.%03d\n", channel+1, red[channel], green[channel], blue[channel]);
	}
	else
	{
		std::cerr << "ERROR: Invalid channel (" << channel << ") passed to displayColorValues(). Ignoring request." << std::endl;
	}
}

void displayCustomColorValues()
{
	if (dynamic_colors)
	{
		std::cout << "Color messages are currently enabled." << std::endl;
	}
	else
	{
		std::cout << "Color messages are currently disabled." << std::endl;
	}

	if (mostRecentColorMessageChannel < 0)
	{
		std::cout << "No color messages have been received yet." << std::endl;
		std::cout << "All channels are set to default color configuration." << std::endl;
	}
	else
	{
		std::cout << "Last color message received was on channel " << mostRecentColorMessageChannel+1 << "." << std::endl;
		return displayColorValues(mostRecentColorMessageChannel);
	}
}

void end(int status)
{
	//clear_LEDs();
	bcm2835_spi_end();
	bcm2835_close();
	delete midiIn;
	exit(status);
}

void end()
{
	end(0);
}

void signal_handler(int signal)
{
	fprintf(stdout, "\n\nReceived signal %d. Send EOF (Ctrl+D) to quit.\n\n", signal);
	std::cout << "Enter (code) to issue command:" << std::endl;
	for (unsigned int i = 0; i < commands.size(); i++)
	{
		char commandChar;
		if (i < 10) 
		{
			commandChar = '0' + i;
		}
		else
		{
			commandChar = 'A' + i-10;
		}
		fprintf(stdout, "(%c) %s\n", commandChar, commands[i]->getName().c_str());
	}
	std::cout << std::endl; 
}

bool colorsLoaded = false;

void loadChannelColorConfig()
{
	mostRecentColorMessageChannel = -1;

	redConfig = new int[numChannels];
	greenConfig = new int[numChannels];
	blueConfig = new int[numChannels];

	red = new int[numChannels];
	green = new int[numChannels];
	blue = new int[numChannels];

	channelIsLowPriority = new bool[numChannels];
	channelNeedsUpdateMessage = new bool[numChannels];
	
	int chnl = 0;
	int r = 0;
	int g = 0;
	int b = 0;
	
	for (int i = 0; i < numChannels; i++)
	{
		redConfig[i] = 32;
		greenConfig[i] = 32;
		blueConfig[i] = 32;

		red[i] = redConfig[i];
		green[i] = greenConfig[i];
		blue[i] = blueConfig[i];

		channelIsLowPriority[i] = false;
		channelNeedsUpdateMessage[i] = false;
	}

	std::ifstream channelMappingFile;
	channelMappingFile.open(channelMappingFilename);
	
	if (channelMappingFile.is_open())
	{
		while ( !channelMappingFile.eof() )
		{
			// Handle channel flags

			bool chnlNeedsUpdateMessage = false;
			bool chnlIsLowPriority = false;

			std::string channelFlags;
			channelMappingFile >> channelFlags;

			if (channelFlags.find("+") != std::string::npos) chnlNeedsUpdateMessage = true;
			if (channelFlags.find("-") != std::string::npos) chnlIsLowPriority = true;


			// Read channel colors

			channelMappingFile >> chnl;
			channelMappingFile >> r;
			channelMappingFile >> g;
			channelMappingFile >> b;

			//std::cout << "chnlNeedsUpdateMessage: " << chnlNeedsUpdateMessage << std::endl;
			//std::cout << "chnlIsLowPriority: " << chnlIsLowPriority << std::endl;
			//std::cout << chnl << " " << r << " " << g << " " << b << "" << std::endl << std::endl;

			if (chnl < 1 || chnl > 16)
			{
				std::cerr << "Error in (" << channelMappingFilename << "): Channel must be between 1 and 16 (was " << chnl << ").\nExiting..." << std::endl;
				end(1);
			}	
			if (r < -MAX_VELOCITY || r > MAX_VELOCITY)
			{
				std::cerr << "Error in (" << channelMappingFilename << "): Red must be between -" << MAX_VELOCITY << " and " << MAX_VELOCITY << " (was " << r << ").\nExiting..." << std::endl;
				end(1);
			}	
			if (g < -MAX_VELOCITY || g > MAX_VELOCITY)
			{
				std::cerr << "Error in (" << channelMappingFilename << "): Green must be between -" << MAX_VELOCITY << " and " << MAX_VELOCITY << " (was " << g << ").\nExiting..." << std::endl;
				end(1);
			}	
			if (b < -MAX_VELOCITY || b > MAX_VELOCITY)
			{
				std::cerr << "Error in (" << channelMappingFilename << "): Blue must be between -" << MAX_VELOCITY << " and " << MAX_VELOCITY << " (was " << b << ").\nExiting..." << std::endl;
				end(1);
			}	
			redConfig[chnl-1] = r;
			greenConfig[chnl-1] = g;
			blueConfig[chnl-1] = b;

			red[chnl-1] = redConfig[chnl-1];
			green[chnl-1] = greenConfig[chnl-1];
			blue[chnl-1] = blueConfig[chnl-1];
			
			channelIsLowPriority[chnl-1] = chnlIsLowPriority;
			channelNeedsUpdateMessage[chnl-1] = chnlNeedsUpdateMessage;
		}
		channelMappingFile.close();

		if (colorsLoaded)
		{
			std::cout << "Successfully loaded MIDI channel mapping config file: " << channelMappingFilename << std::endl;
			std::cout << "All channels reset to default color configuration." << std::endl;
		}

		colorsLoaded = true;
	}
	
	else
	{
		std::cerr << "Could not open MIDI channel mapping config file: " << channelMappingFilename << "\nUsing all white." << std::endl;
	}
}

void clearALSAConnections()
{
	std::string command = "aconnect -x";
	system(command.c_str());

	std::cout << "All ALSA ports have been disconnected using aconnect." << std::endl;
}

void createCommandList()
{
	commands.push_back(new Command("Clear LEDs", &clear_LEDs));
	commands.push_back(new Command("Dump Data Structures", &dumpDataStructures));
	commands.push_back(new Command("Toggle MIDI data dump", &toggleDataDump));
	commands.push_back(new Command("Toggle Custom Color Messages", &toggleDynamicColors));
	commands.push_back(new Command("Display Custom Color Values", &displayCustomColorValues));
	commands.push_back(new Command("Display Custom Color MIDI Message Info", &displayColorMessageCodes));
	commands.push_back(new Command("Reload MIDI Channel Config and Reset Custom Colors", &loadChannelColorConfig));
	commands.push_back(new Command("Clear ALSA Connections", &clearALSAConnections));
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
		if ( !noteMappingFile.eof() ) noteMappingFile >> lowestNoteOnPiano;
		if ( !noteMappingFile.eof() ) noteMappingFile >> highestNoteOnPiano;

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

	
	damperActive = new bool[numChannels];
	sostenutoActive = new bool[numChannels];

	for (int i = 0; i < numChannels; i++)
	{
		damperActive[i] = false;
		sostenutoActive[i] = false;
	}
	
	
	loadChannelColorConfig();

	
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
	
	midiIn = new RtMidiIn(RtMidi::Api::UNSPECIFIED, DEFAULT_RTMIDI_NAME, 100);
	
	// Create a port that other audio software can output MIDI data to
	midiIn->openVirtualPort();
	
	// Set our callback function.  This should be done immediately after
	// opening the port to avoid having incoming messages written to the
	// queue.
	midiIn->setCallback( &onMidiMessageReceived );
	
	// ignore sysex, timing, or active sensing messages.
	midiIn->ignoreTypes( true, true, true );

	lastMidiMessageReceived = new std::vector<unsigned char>();

	
	// Connect ALSA Ports
	if (autoConnectALSAPorts)
	{
		std::string command = "aconnect \"" + DEFAULT_ALSA_INPUT_NAME + "\" \"" + DEFAULT_ALSA_OUTPUT_NAME + "\"";
		system(command.c_str());
	}

	fprintf(stdout, "\nReady to interpret MIDI input data.\n\n");
}

void loop()
{
	char input;
	while (std::cin.get(input))
	{
		int commandIndex = -1;
		if (input >= '0' && input <= '9')
		{
			commandIndex = input - '0';
		}
		else if (input >= 'A' && input <= 'Z')
		{
			commandIndex = input - 'A' + 10;
		}
		else if (input >= 'a' && input <= 'z')
		{
			commandIndex = input - 'a' + 10;
		}
		if (commandIndex >= 0)
		{
			Command* command = commands[commandIndex];
			funcPtr commandFunctionPointer = command->getFunctionPointer();
			std::cout << "Executing command: " << command->getName() << std::endl;
			commandFunctionPointer();
			std::cout << std::endl;
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
