all:
	g++ -Wall -D__LINUX_ALSA__ main.cpp -o midiLEDs -l bcm2835 -l rtmidi -l asound -l pthread 
