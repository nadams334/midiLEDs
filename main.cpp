#include "RtMidi.h"
#include <stdio.h>
int main()
{
  RtMidiIn *midiin = 0;
  // RtMidiIn constructor
  try {
    midiin = new RtMidiIn();
  }
  catch (RtMidiError &error) {
    // Handle the exception here
    error.printMessage();
  }
  while (getchar() != EOF);
  // Clean up
  delete midiin;
}
