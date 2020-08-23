/*
 *Author : Vishnu Saradhara 
 *Date   : 10/08/2020
 */
float power(float x, int y)
{
  float temp;
  if (y == 0)
    return 1;
  temp = power(x, y / 2);
  if (y % 2 == 0)
    return temp * temp;
  else
  {
    if (y > 0)
      return x *temp * temp;
    else
      return (temp *temp) / x;
  }
}

float calculate_distance(int tx_power, int rssi)
{
  float A = 0.950827299;
  float B = 4.61399983;
  float C = 0.06503583;
  if (rssi == 0)
  {
    return -1.0;
  }

  if (tx_power == 0)
  {
    return -1.0;
  }

  float ratio = rssi *1.0 / tx_power;
  if (ratio < 1.0)
  {
    return power(ratio, 10);
  }

  float distance = (A) *(power(ratio, B)) + C;
  return distance;
}
