#include <iostream>
#include <fstream>
using namespace std;

int naturalOffset = 134;
int accidentalOffset = 155;
int LEDsPerOctave = 24;
int numFullOctaves = 5;

int main()
{

	ofstream myfile;
	myfile.open("pianoLEDmapping.cfg");

	for (int i = 0; i < numFullOctaves; i++)
	{
		myfile << 0+12*i << " " << (naturalOffset - LEDsPerOctave * i) - 0 << endl;
		myfile << 1+12*i << " " << (accidentalOffset + LEDsPerOctave * i)  + 0 << endl;
		myfile << 2+12*i << " " << (naturalOffset - LEDsPerOctave * i) - 4 << endl;
		myfile << 3+12*i << " " << (accidentalOffset + LEDsPerOctave * i)  + 4 << endl;
		myfile << 4+12*i << " " << (naturalOffset - LEDsPerOctave * i) - 8 << endl;
		myfile << 5+12*i << " " << (naturalOffset - LEDsPerOctave * i) - 10 << endl;
		myfile << 6+12*i << " " << (accidentalOffset + LEDsPerOctave * i)  + 10 << endl;
		myfile << 7+12*i << " " << (naturalOffset - LEDsPerOctave * i) - 14 << endl;
		myfile << 8+12*i << " " << (accidentalOffset + LEDsPerOctave * i)  + 14 << endl;
		myfile << 9+12*i << " " << (naturalOffset - LEDsPerOctave * i) - 18 << endl;
		myfile << 10+12*i << " " << (accidentalOffset + LEDsPerOctave * i)  + 18 << endl;
		myfile << 11+12*i << " " << (naturalOffset - LEDsPerOctave * i) - 22 << endl;
	}

	myfile << 0+12*numFullOctaves << " " << (naturalOffset - LEDsPerOctave * numFullOctaves) - 0;

	myfile.close();
	return 0;

}