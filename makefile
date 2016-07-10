all:
	g++ -Wall -D__LINUX_ALSA__ main.cpp -l rtmidi -l asound -l pthread -o midiLEDs
