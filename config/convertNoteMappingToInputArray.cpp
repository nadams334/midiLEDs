#include <iostream>
#include <fstream>
#include <deque>

using namespace std;


const int numLEDs = 288;
const int brightness = 1;
const int red = 255;
const int green = 0;
const int blue = 255;


int main()
{

	ifstream inFile;
	inFile.open("noteMapping.cfg");

	ofstream outFile;
	outFile.open("all_keyboards_notes.apa");

	deque<int> LEDs;
	int burnFirstInt;
	inFile >> burnFirstInt;

	for (int nextInt; inFile >> nextInt; inFile >> nextInt) // grab every second int
	{
		LEDs.push_back(nextInt);
	}

	for (int i = 0; i < numLEDs; i++)
	{
		if (i == LEDs.front())
		{
			outFile << brightness << " " << red << " " << green << " " << blue << endl;
			LEDs.pop_front();
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