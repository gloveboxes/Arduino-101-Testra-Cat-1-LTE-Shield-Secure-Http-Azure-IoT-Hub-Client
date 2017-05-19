#include "Cat1shield.h"

void Cat1Shield::measure()
{
  temperature = pressure = humidity = 0;
  char temp[20];

  shield->getTemperature(temp);
  temperature = atof(temp);

  shield->getLightLevel(temp);
  light = atoi(temp);
}
