#include <iostream>
#include <fstream>
using namespace std;





// LED strip config


// Strip sizing
const int LEDsPerOctave = 24; // how many LEDs to reach from one C to the next

// Strip placement
const int naturalOffset = 134; // what is the number of the LED corresponding to the high C
const int accidentalOffset = 155; // what is the number of the LED corresponding to the low C#

// Generic indicator LEDs
const int topLeftIndicator = 149;
const int topRightIndicator = 277;
const int bottomLeftIndicator = 138;
const int bottomRightIndicator = 10;





// Piano Config


// Full octaves
const int firstOctave = 3; // Octave of the lowest C on your piano
const int numFullOctaves = 5; // Number of C->C octaves

// Partial octaves
const int numKeysBeforeFirstC = 0; // How many keys before the lowest C on your piano (eg. G = 5: B, Bb, A, Ab, and G)
const int numKeysAfterLastB = 1; // How many keys go past the highest B on your piano (eg. D = 3: C, C#, and D)





int main()
{

	ofstream myfile;
	myfile.open("noteMapping.cfg");

	for (int i = -1; i <= numFullOctaves; i++)
	{

/* C: */	if (  ( i >= 0 && i < numFullOctaves ) || ( i < 0 && numKeysBeforeFirstC >= 12 ) || ( i == numFullOctaves && numKeysAfterLastB >= 1 )  )

			myfile << 0+12*(i+firstOctave) << " " << (naturalOffset - LEDsPerOctave * i) - 0 << endl;

/* C#: */	if (  ( i >= 0 && i < numFullOctaves ) || ( i < 0 && numKeysBeforeFirstC >= 11 ) || ( i == numFullOctaves && numKeysAfterLastB >= 2 )  )

			myfile << 1+12*(i+firstOctave) << " " << (accidentalOffset + LEDsPerOctave * i)  + 0 << endl;

/* D: */	if (  ( i >= 0 && i < numFullOctaves ) || ( i < 0 && numKeysBeforeFirstC >= 10 ) || ( i == numFullOctaves && numKeysAfterLastB >= 3 )  )

			myfile << 2+12*(i+firstOctave) << " " << (naturalOffset - LEDsPerOctave * i) - 4 << endl;

/* D#: */	if (  ( i >= 0 && i < numFullOctaves ) || ( i < 0 && numKeysBeforeFirstC >= 9 ) || ( i == numFullOctaves && numKeysAfterLastB >= 4 )  )

			myfile << 3+12*(i+firstOctave) << " " << (accidentalOffset + LEDsPerOctave * i)  + 4 << endl;

/* E: */	if (  ( i >= 0 && i < numFullOctaves ) || ( i < 0 && numKeysBeforeFirstC >= 8 ) || ( i == numFullOctaves && numKeysAfterLastB >= 5 )  )

			myfile << 4+12*(i+firstOctave) << " " << (naturalOffset - LEDsPerOctave * i) - 8 << endl;

/* F: */	if (  ( i >= 0 && i < numFullOctaves ) || ( i < 0 && numKeysBeforeFirstC >= 7 ) || ( i == numFullOctaves && numKeysAfterLastB >= 6 )  )

			myfile << 5+12*(i+firstOctave) << " " << (naturalOffset - LEDsPerOctave * i) - 10 << endl;

/* F#: */	if (  ( i >= 0 && i < numFullOctaves ) || ( i < 0 && numKeysBeforeFirstC >= 6 ) || ( i == numFullOctaves && numKeysAfterLastB >= 7 )  )

			myfile << 6+12*(i+firstOctave) << " " << (accidentalOffset + LEDsPerOctave * i)  + 10 << endl;

/* G: */	if (  ( i >= 0 && i < numFullOctaves ) || ( i < 0 && numKeysBeforeFirstC >= 5 ) || ( i == numFullOctaves && numKeysAfterLastB >= 8 )  )

			myfile << 7+12*(i+firstOctave) << " " << (naturalOffset - LEDsPerOctave * i) - 14 << endl;

/* G#: */	if (  ( i >= 0 && i < numFullOctaves ) || ( i < 0 && numKeysBeforeFirstC >= 4 ) || ( i == numFullOctaves && numKeysAfterLastB >= 8 )  )

			myfile << 8+12*(i+firstOctave) << " " << (accidentalOffset + LEDsPerOctave * i)  + 14 << endl;

/* A: */	if (  ( i >= 0 && i < numFullOctaves ) || ( i < 0 && numKeysBeforeFirstC >= 3 ) || ( i == numFullOctaves && numKeysAfterLastB >= 10 )  )

			myfile << 9+12*(i+firstOctave) << " " << (naturalOffset - LEDsPerOctave * i) - 18 << endl;

/* A#: */	if (  ( i >= 0 && i < numFullOctaves ) || ( i < 0 && numKeysBeforeFirstC >= 2 ) || ( i == numFullOctaves && numKeysAfterLastB >= 11 )  )

			myfile << 10+12*(i+firstOctave) << " " << (accidentalOffset + LEDsPerOctave * i)  + 18 << endl;

/* B: */	if (  ( i >= 0 && i < numFullOctaves ) || ( i < 0 && numKeysBeforeFirstC >= 1 ) || ( i == numFullOctaves && numKeysAfterLastB >= 12 )  )

			myfile << 11+12*(i+firstOctave) << " " << (naturalOffset - LEDsPerOctave * i) - 22 << endl;

	}

	myfile.close();
	return 0;

}