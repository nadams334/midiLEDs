#include <iostream>
#include <fstream>
#include <set>

using namespace std;


const int numLEDs = 288;
const int brightness = 1;
const int red = 255;
const int green = 0;
const int blue = 0;


int main()
{

	ifstream inFile;
	inFile.open("noteMapping.cfg");

	ofstream outFile;
	outFile.open("all_keyboard_notes.apa");

	set<int> LEDs;

	int nextInt;
	while (inFile >> nextInt && inFile >> nextInt) // read every second integer only, these represent the LED number
	{
		LEDs.insert(nextInt);
	}

	for (int i = 0; i < numLEDs; i++)
	{
		set<int>::iterator it = LEDs.find(i);
		if (it != LEDs.end())
		{
			outFile << brightness << " " << red << " " << green << " " << blue << endl;
			LEDs.erase(it);
		}
		else
		{
			outFile << "0 0 0 0" << endl;
		}
	}

	inFile.close();
	outFile.close();
	return 0;

}