all:
	g++ -std=c++11 -Wall -D __LINUX_ALSA__ main.cpp -o midiLEDs -l bcm2835 -l rtmidi -l asound -l pthread
