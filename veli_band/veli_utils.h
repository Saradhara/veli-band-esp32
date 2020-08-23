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

uint16_t uuid_to_uint_16(){
  char *uuid = "3bdb098f-b8b0-4d1b-baa2-0d93eb7169c4";
  static uint8_t adv_data[32];  //your uuid has 32 byte of data
  int strCounter=0;      // need two counters: one for uuid string (size=38) and
  int hexCounter=0;      // another one for destination adv_data (size=32)
  while (strCounter<strlen(uuid))
  {
       if (uuid[strCounter] == '-') 
       {
           strCounter++;     //go to the next element
           continue;
       }
  
       // convert the character to string
       char str[2] = "\0";
       str[0] = uuid[strCounter];
  
       // convert string to int base 16
       adv_data[hexCounter]= (uint8_t)atoi(str);
  
       strCounter++;
       hexCounter++;
  }
  uint16_t u_Int = *( uint16_t* ) adv_data;
  return u_Int;
}
