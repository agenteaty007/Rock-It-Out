/*
Project: RockItOut_audio_processor
Author: Alberto Tam Yong
Date: 06/05/2015
*/

unsigned short samples_taken = 20;
unsigned int reading_zero = 300;
unsigned int reading_peak = 330;
unsigned int reading_bottom = 270;
unsigned short frequency_calculated[20];
unsigned int f_c_index = 0;
unsigned long init_time = 0;
unsigned long middle_time = 0;
unsigned long end_time = 0;

unsigned short sample_array1[400];
unsigned short sample_array1_size = 400;

enum SM1_states {SM1_start,SM1_wait,SM1_readPeak,SM1_readNode,SM1_readBottom,SM1_readNode2} SM1_state;

void Tick1()
{
  unsigned int reading_A0;
  switch(SM1_state)
  {
    case SM1_start:
      SM1_state = SM1_wait;
      break;
    case SM1_wait:
      reading_A0 = analogRead(A0);
      if(reading_A0 == reading_zero)
        SM1_state = SM1_readPeak;
      else
        SM1_state = SM1_wait;
      break;
    case SM1_readPeak:
      reading_A0 = analogRead(A0);
      if(reading_A0 == reading_peak)
        SM1_state = SM1_readNode;
      else if(reading_A0 > reading_zero)
        SM1_state = SM1_readPeak;
      else if(reading_A0 < reading_zero)
        SM1_state = SM1_wait;
      break;
    case SM1_readNode:
      reading_A0 = analogRead(A0);
      if(reading_A0 == reading_zero)
        SM1_state = SM1_readBottom;
      else if(reading_A0 > reading_zero)
        SM1_state = SM1_readNode;
      else if(reading_A0 < reading_zero)
        SM1_state = SM1_wait;
      break;
    case SM1_readBottom:
      reading_A0 = analogRead(A0);
      if(reading_A0 == reading_bottom)
        SM1_state = SM1_readNode2;
      else if(reading_A0 < reading_zero)
        SM1_state = SM1_readBottom;
      else if(reading_A0 > reading_zero)
        SM1_state = SM1_wait;
      break;
    case SM1_readNode2:
      reading_A0 = analogRead(A0);
      if(reading_A0 == reading_zero)
        SM1_state = SM1_wait;
      else if(reading_A0 < reading_zero)
        SM1_state = SM1_readNode2;
      else if(reading_A0 > reading_zero)
        SM1_state = SM1_wait;
      break;
    default:
      break;
  }
  switch(SM1_state)
  {
    case SM1_wait:
      init_time = millis();
      break;
    case SM1_readNode:
      middle_time = millis();
      break;
    case SM1_readNode2:
      end_time = millis();
      frequency_calculated[f_c_index] = end_time - init_time;
      if(f_c_index < (samples_taken-1))
        f_c_index++;
      else
        f_c_index = 0;
      break;
    default:
      break;
  }
}

void sample(unsigned long sample_time)
{
  unsigned long prev_time = millis();
  while(millis() - prev_time < sample_time)
  {
    Tick1();
  }
}

void sample2(unsigned long sample_time,unsigned short sample_array[])
{
  unsigned long prev_time = millis();
  unsigned long index = 0;
  while(millis() - prev_time < sample_time)
  {
    if(index < sample_array1_size)
    {
      sample_array[index] = analogRead(A0);
      index++;
    }
    else
    {
      break;
    }
  }
}

void display_sampled_data()
{
  for(int i = 0; i < samples_taken; i++)
  {
    Serial.print(i);
    Serial.print("\t");
    Serial.print(frequency_calculated[i]);
    Serial.println();
  }
}

void display_sampled_array(unsigned short sample_array[], unsigned short array_size)
{
  for(int i = 0; i < array_size; i++)
  {
    Serial.print(i);
    Serial.print(" ");
    Serial.println(sample_array[i]);
  }
}

void calculate_delta_pos(unsigned short sample_array[],unsigned short &array_size)
{
  for(int i = 1; i < array_size; i++)
  {
    sample_array[i-1] = sample_array[i] - sample_array[i-1];
    if(i == array_size-1)
    {
      sample_array[i] = 0;
      array_size--;
    }
  }
}

unsigned short total_readings = 0;

unsigned short math_frequency(unsigned short sample_array[],unsigned short sample_size,unsigned short total_samples,unsigned short duration)
{
  unsigned long result = 0;
  for(int i = 0; i < sample_size; i++)
  {
    result += sample_array[i];
  }
  result = result/sample_size;
  result = (1000*long(total_samples))/result;
  result = result/duration;
  return result;
}

unsigned short sample_max_pos[50];
unsigned short sample_min_pos[50];
unsigned short new_sample_max_pos[30];
unsigned short new_sample_min_pos[30];

unsigned short calculate_frequency_sampled_array(unsigned short sample_array[], unsigned short array_size)
{
  unsigned short sample_max = 0;
  unsigned short sample_min = 1024;
  unsigned short sample_max_pos_index = 0;
  unsigned short sample_min_pos_index = 0;
  unsigned short new_sample_max_pos_index = 0;
  unsigned short new_sample_min_pos_index = 0;
  
  unsigned short array_max_size = 50;
  unsigned short array_min_size = 50;
  
  unsigned char sample_margin_error = 5;
  unsigned char peak_debounce = 5;
  
  //Find max peak and min peak
  for(int i = 0; i < array_size; i++)
  {
    if(sample_array[i] < sample_min && sample_array[i] != 0)
    {
      sample_min = sample_array[i];
    }
    if(sample_array[i] > sample_max && sample_array[i] != 0)
    {
      sample_max = sample_array[i];
    }
  }
  
  //Store on memory the max and min peaks
  for(int i = 0; i < array_size; i++)
  {
    if(sample_array[i] <= sample_max + sample_margin_error && 
    sample_array[i] >= sample_max - sample_margin_error)
    {
      sample_max_pos[sample_max_pos_index] = i;
      sample_max_pos_index++;
    }
    
    if(sample_array[i] <= sample_min + sample_margin_error && 
    sample_array[i] >= sample_min - sample_margin_error)
    {
      sample_min_pos[sample_min_pos_index] = i;
      sample_min_pos_index++;
    }
  }
  
  /*
  //Cut array down to fit only necessary spaces
  for(int i = 0; i < array_max_size; i++)
  {
    if(sample_max_pos[i] == 0)
    {
      array_max_size = i;
      break;
    }
  }
  
  for(int i = 0; i < array_min_size; i++)
  {
    if(sample_min_pos[i] == 0)
    {
      array_min_size = i;
      break;
    }
  }
  */
  
  /*
  Serial.println("---------------------------------------");
  Serial.println("MAX");
  display_sampled_array(sample_max_pos,sample_max_pos_index);
  Serial.println("---------------------------------------");
  Serial.println("MIN");
  display_sampled_array(sample_min_pos,sample_min_pos_index);
  Serial.println("---------------------------------------");
  */
  
  //Look for redundant and repeated peak readings and only store peak
  unsigned short temp = 0;
  
  for(int i = 1; i < sample_max_pos_index; i++)
  {
    if(sample_max_pos[i] - sample_max_pos[i-1] <= peak_debounce)
    {
      temp++;
    }
    else
    {
      if(temp > 0)
      {
        new_sample_max_pos[new_sample_max_pos_index] = sample_max_pos[(i-(temp/2))];
        new_sample_max_pos_index++;
        temp = 0;
      }
      else
      {
        new_sample_max_pos[new_sample_max_pos_index] = sample_max_pos[i];
        new_sample_max_pos_index++;
      }
    }
  }
  
  temp = 0;
  for(int i = 1; i < sample_min_pos_index; i++)
  {
    if(sample_min_pos[i] - sample_min_pos[i-1] == 1)
    {
      temp++;
    }
    else
    {
      if(temp > 0)
      {
        new_sample_min_pos[new_sample_min_pos_index] = sample_min_pos[(i-(temp/2))];
        new_sample_min_pos_index++;
        temp = 0;
      }
      else
      {
        new_sample_min_pos[new_sample_min_pos_index] = sample_min_pos[i];
        new_sample_min_pos_index++;
      }
    }
  }
  /*
  Serial.println("---------------------------------------");
  Serial.println("MAX (w/ redundancies)");
  display_sampled_array(new_sample_max_pos,new_sample_max_pos_index);
  Serial.println("---------------------------------------");
  Serial.println("MIN (w/ redundancies)");
  display_sampled_array(new_sample_min_pos,new_sample_min_pos_index);
  Serial.println("---------------------------------------");
  */
  
  calculate_delta_pos(new_sample_max_pos,new_sample_max_pos_index);
  calculate_delta_pos(new_sample_min_pos,new_sample_min_pos_index);
  
  /*
  Serial.println("---------------------------------------");
  Serial.println("MAX Delta");
  display_sampled_array(new_sample_max_pos,new_sample_max_pos_index);
  Serial.println("---------------------------------------");
  Serial.println("MIN Delta");
  display_sampled_array(new_sample_min_pos,new_sample_min_pos_index);
  Serial.println("---------------------------------------");
  
  Serial.println("---------------------------------------");
  Serial.print("Max Frequecny: ");
  Serial.println(math_frequency(new_sample_max_pos,new_sample_max_pos_index,total_readings,25));
  Serial.println("---------------------------------------");
  Serial.print("MIN Frequency: ");
  Serial.println(math_frequency(new_sample_min_pos,new_sample_min_pos_index,total_readings,25));
  Serial.println("---------------------------------------");
  */
  return math_frequency(new_sample_min_pos,new_sample_min_pos_index,total_readings,25);
  //return math_frequency(new_sample_max_pos,new_sample_max_pos_index,total_readings,25);
}

void live_preview_sample_array(unsigned short sample_array[], unsigned short array_size)
{
  unsigned short sample_max = 0;
  unsigned short sample_min = 1024;
  unsigned short sample_max_pos_index = 0;
  unsigned short sample_min_pos_index = 0;
  
  for(int i = 0; i < array_size; i++)
  {
    if(sample_array[i] == 0)
    {
      array_size = i;
    }
  }
  
  total_readings = array_size;
  
  for(int i = 0; i < array_size; i++)
  {
    if(sample_array[i] < sample_min && sample_array[i] != 0)
    {
      sample_min = sample_array[i];
    }
    if(sample_array[i] > sample_max && sample_array[i] != 0)
    {
      sample_max = sample_array[i];
    }
  }
  
  for(int i = sample_max; i >= sample_min; i--)
  {
    Serial.print(i); //assuming i will always be 3 digits
    Serial.print(" ");
    for(int j = 0; j < array_size; j++)
    {
      if(sample_array[j] == i)
      {
        Serial.print("o");
      }
      else
      {
        Serial.print("-");
      }
    }
    Serial.println();
  }
  
  Serial.print("    ");
  for(int i = 0; i < array_size; i++)
  {
    Serial.print(i/100);
  }
  Serial.println();
  Serial.print("    ");
  for(int i = 0; i < array_size; i++)
  {
    Serial.print((i%100)/10);
  }
  Serial.println();
  Serial.print("    ");
  for(int i = 0; i < array_size; i++)
  {
    Serial.print(i%10);
  }
  Serial.println();
  
  Serial.println();
  Serial.println();
}

unsigned long prev_time_display_frequency_sampled = millis();

void display_frequency_sampled(unsigned long sample_time)
{
  if(millis() - prev_time_display_frequency_sampled > sample_time)
  {
    unsigned int output = calculate_frequency_sampled_array(sample_array1,sample_array1_size);
    Serial.write(output%256);
    Serial.write((output/256)%256);
    prev_time_display_frequency_sampled = millis();
  }
}

void init_array(unsigned short sample_array[],unsigned short array_size)
{
  for(int i = 0; i < array_size; i++)
  {
    sample_array[i] = 0;
  }
}

void setup() {
  Serial.begin(9600);
  
  pinMode(2,INPUT_PULLUP);
  pinMode(3,INPUT_PULLUP);
  pinMode(4,INPUT_PULLUP);
  pinMode(5,INPUT_PULLUP);
  
  pinMode(11,OUTPUT);
  pinMode(12,OUTPUT);
  pinMode(13,OUTPUT);
  
  init_array(frequency_calculated,samples_taken-1);
  init_array(sample_array1,sample_array1_size);
}

void loop() {
  if(digitalRead(2) == 0)
  {
    digitalWrite(13,HIGH);
    delay(2000);
    digitalWrite(13,LOW);
    sample2(50,sample_array1);
  }
  if(digitalRead(3) == 0)
  {
    display_sampled_array(sample_array1,sample_array1_size);
    delay(1000);
  }
  //if(digitalRead(4) == 0)
  if(true)
  {
    display_frequency_sampled(500);
  }
  //if(digitalRead(5) == 0)
  if(true)
  {
    sample2(25,sample_array1);
    //delay(10);
    //live_preview_sample_array(sample_array1,sample_array1_size);
    //delay(100);
  }
}
