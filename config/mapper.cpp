#include <iostream>
#include <fstream>
using namespace std;





// LED strip config


// Strip sizing
const int LEDsPerOctave = 24; // how many LEDs to reach from one C to the next
const int numLEDs = 288;

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
const int numFullOctaves = 5; // Number of C->B octaves

// Partial octaves
const int numKeysBeforeFirstC = 0; // How many keys before the lowest C on your piano (eg. G = 5: B, Bb, A, Ab, and G)
const int numKeysAfterLastB = 1; // How many keys go past the highest B on your piano (eg. D = 3: C, C#, and D)



// Mapping

const int numMidiNotes = 128;
int* noteMapping;


int main()
{	
	noteMapping = new int[numMidiNotes];

	// Initialize all notes to their appropriate indicator LEDs

	for (int i = 0; i < numMidiNotes; i++)
	{
		bool natural;
		bool leftIndicator;
		int indicator;

		// Determine if note is natural or accidental

		if (i % 12 == 0 || i % 12 == 2 || i % 12 == 4 || i % 12 == 5 || i % 12 == 7 || i % 12 == 9 || i % 12 == 11)
		{
			natural = true;
		}
		else
		{
			natural = false;
		}


		// Determine if note is lower than first 		

		if (i / 12 < firstOctave)
		{
			leftIndicator = true;
		}
		else
		{
			leftIndicator = false;
		}

		if (!natural && !leftIndicator)
		{
			indicator = topRightIndicator;
		}
		else if (!natural && leftIndicator)
		{
			indicator = topLeftIndicator;
		}
		else if (natural && !leftIndicator)
		{
			indicator = bottomRightIndicator;
		}
		else // natural && leftIndicator
		{
			indicator = bottomLeftIndicator;
		}

		noteMapping[i] = indicator;
	}


	// Map all notes the piano has keys for

	for (int i = -1; i <= numFullOctaves; i++)
	{

/* C: */	if (  ( i >= 0 && i < numFullOctaves ) || ( i < 0 && numKeysBeforeFirstC >= 12 ) || ( i == numFullOctaves && numKeysAfterLastB >= 1 )  )

			noteMapping[0+12*(i+firstOctave)] = (naturalOffset - LEDsPerOctave * i) - 0;

/* C#: */	if (  ( i >= 0 && i < numFullOctaves ) || ( i < 0 && numKeysBeforeFirstC >= 11 ) || ( i == numFullOctaves && numKeysAfterLastB >= 2 )  )

			noteMapping[1+12*(i+firstOctave)] = (accidentalOffset + LEDsPerOctave * i)  + 0;

/* D: */	if (  ( i >= 0 && i < numFullOctaves ) || ( i < 0 && numKeysBeforeFirstC >= 10 ) || ( i == numFullOctaves && numKeysAfterLastB >= 3 )  )

			noteMapping[2+12*(i+firstOctave)] = (naturalOffset - LEDsPerOctave * i) - 4;

/* D#: */	if (  ( i >= 0 && i < numFullOctaves ) || ( i < 0 && numKeysBeforeFirstC >= 9 ) || ( i == numFullOctaves && numKeysAfterLastB >= 4 )  )

			noteMapping[3+12*(i+firstOctave)] = (accidentalOffset + LEDsPerOctave * i)  + 4;

/* E: */	if (  ( i >= 0 && i < numFullOctaves ) || ( i < 0 && numKeysBeforeFirstC >= 8 ) || ( i == numFullOctaves && numKeysAfterLastB >= 5 )  )

			noteMapping[4+12*(i+firstOctave)] = (naturalOffset - LEDsPerOctave * i) - 8;

/* F: */	if (  ( i >= 0 && i < numFullOctaves ) || ( i < 0 && numKeysBeforeFirstC >= 7 ) || ( i == numFullOctaves && numKeysAfterLastB >= 6 )  )

			noteMapping[5+12*(i+firstOctave)] = (naturalOffset - LEDsPerOctave * i) - 10;

/* F#: */	if (  ( i >= 0 && i < numFullOctaves ) || ( i < 0 && numKeysBeforeFirstC >= 6 ) || ( i == numFullOctaves && numKeysAfterLastB >= 7 )  )

			noteMapping[6+12*(i+firstOctave)] = (accidentalOffset + LEDsPerOctave * i)  + 10;

/* G: */	if (  ( i >= 0 && i < numFullOctaves ) || ( i < 0 && numKeysBeforeFirstC >= 5 ) || ( i == numFullOctaves && numKeysAfterLastB >= 8 )  )

			noteMapping[7+12*(i+firstOctave)] = (naturalOffset - LEDsPerOctave * i) - 14;

/* G#: */	if (  ( i >= 0 && i < numFullOctaves ) || ( i < 0 && numKeysBeforeFirstC >= 4 ) || ( i == numFullOctaves && numKeysAfterLastB >= 8 )  )

			noteMapping[8+12*(i+firstOctave)] = (accidentalOffset + LEDsPerOctave * i)  + 14;

/* A: */	if (  ( i >= 0 && i < numFullOctaves ) || ( i < 0 && numKeysBeforeFirstC >= 3 ) || ( i == numFullOctaves && numKeysAfterLastB >= 10 )  )

			noteMapping[9+12*(i+firstOctave)] = (naturalOffset - LEDsPerOctave * i) - 18;

/* A#: */	if (  ( i >= 0 && i < numFullOctaves ) || ( i < 0 && numKeysBeforeFirstC >= 2 ) || ( i == numFullOctaves && numKeysAfterLastB >= 11 )  )

			noteMapping[10+12*(i+firstOctave)] = (accidentalOffset + LEDsPerOctave * i)  + 18;

/* B: */	if (  ( i >= 0 && i < numFullOctaves ) || ( i < 0 && numKeysBeforeFirstC >= 1 ) || ( i == numFullOctaves && numKeysAfterLastB >= 12 )  )

			noteMapping[11+12*(i+firstOctave)] = (naturalOffset - LEDsPerOctave * i) - 22;

	}


	// write mapping to file
	
	ofstream mappingFile;
	mappingFile.open("noteMapping.cfg");

	for (int i = 0; i < numMidiNotes; i++) 
	{
		mappingFile << i << " " << noteMapping[i] << endl;
	}

	mappingFile.close();


	//end

	return 0;
}