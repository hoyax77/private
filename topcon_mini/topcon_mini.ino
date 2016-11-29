#define DEBUG

//non-initialized value
//union configChannel{
typedef struct configChannel{  
  uint8_t enable[10];
  uint16_t risingtime[10];
  uint16_t dispensingtime[10];
  uint16_t exhausttime[10];
  uint16_t highvactime[10];
  uint16_t dispensingpressure[10];  //unit; 100pa, 3276.8kpa
  uint16_t highvacpressure[10];  //unit: 100pa 
  uint16_t lowvacpressure[10];  //unit: 1pa
  uint8_t risingmode[10];
  uint8_t pulsemode[10];
//} channel __attribute__ ((section (".noinit")));
}channel_t;

typedef struct configMonitoring{
  uint32_t time;
  int32_t pressure;  //unit: 100pa
  uint8_t dispensingflag;
} monitoring_t;

typedef struct configState{
  uint8_t state;
  uint8_t channel;  //0~9
} state_t;

String inputString = "";
uint8_t inputComplete = 0;
channel_t channel;
monitoring_t monitoring;
state_t state;

void setup() {
  // put your setup code here, to run once:
  // Timer0 is already used for millis() - we'll just interrupt somewhere
  // in the middle and call the "Compare A" function below
  OCR0A = 0xAF;
  TIMSK0 |= _BV(OCIE0A);
  
  state.state = 0;
  state.channel = 0;  
  monitoring.time = 0;
  monitoring.pressure = 0;
  monitoring.dispensingflag = 0;

#ifdef DEBUG
  Serial.begin(115200);
  inputString.reserve(10);

  //set-up test channel
  channel.enable[0] = 1;
  channel.risingmode[0] = 1;
  channel.pulsemode[0] = 0;  
  channel.risingtime[0] = 100;
  channel.exhausttime[0] = 300;
  channel.highvactime[0] = 300;
  channel.enable[1] = 1;
  channel.risingmode[1] = 0;
  channel.pulsemode[1] = 1;  
  channel.dispensingtime[1] = 500;
  channel.risingtime[1] = 100;
  channel.exhausttime[1] = 5000;
  channel.highvactime[1] = 1000;
#endif
  Serial.println("System Ready!!!");

}

// Interrupt is called once a millisecond
SIGNAL(TIMER0_COMPA_vect)
{ 
 #ifdef DEBUG 
//  unsigned long currentMillis = millis();
//  Serial.println(currentMillis);
#endif
  _do_processing();
}

void loop() {
  // put your main code here, to run repeatedly:  
#ifdef DEBUG
  if(inputComplete == 1)
  {
    if(inputString.equals("disp on")==1)
    {
      monitoring.dispensingflag = 1;
      Serial.println("CMD: Dispensing On");
      inputComplete = 0;      
    }
    else if(inputString.equals("disp off")==1)
    {
      monitoring.dispensingflag = 0;
      Serial.println("CMD: Dispensing Off");
      inputComplete = 0;
    }
    else if(inputString.equals("load channel 1")==1)
    {
      state.channel = 1;
      Serial.println("CMD: Ch1 Loaded");
      inputComplete = 0;
    }
    else if(inputString.equals("load channel 0")==1)
    {
      state.channel = 0;
      Serial.println("CMD: Ch0 Loaded");
      inputComplete = 0;
    }
    inputString = "";
    inputComplete = 0;
  }
#endif

}
#ifdef DEBUG
void serialEvent()
{
  while(Serial.available())
  {
    char inChar = (char)Serial.read();
    
    if(inChar == '\n')
    {
      inputComplete = 1;  
      Serial.println(inputString);    
    }
    else
    {
      inputString += inChar;
    }
  }
}
#endif
void _digital_write(byte pinNo, byte On)
{
}
void _digital_read(byte pinNo, byte* On)
{
  *On = 1;
}
void _analog_write(byte pinNo, int Digit)
{
}
void _analog_read(byte pinNo, int* Digit)
{
  *Digit = 0;
}
void _relay_write(byte pinNo, byte On)
{
}

void _get_pressure(int32_t* pressure)
{
  int digit;
  _analog_read(0, &digit);
  *pressure = digit/4096 * 900000;  // 900kpa 
}
uint8_t _check_dispensing_on()
{
  //_digital_read(), ethernet comm.
  if(monitoring.dispensingflag == 1)
  {
    return 1;
  }
  else
  {
    return 0;
  }
}
void _set_solenoid_control(byte cda, byte exhaust, byte highvac, byte lowvac)
{
  _relay_write(0, cda);
  _relay_write(1, exhaust);
  _relay_write(2, highvac);
  _relay_write(3, lowvac);
}
void _set_cda_regulator(uint16_t pressure)  //unit 100pa
{
  //_analog_write()
}
void _set_vac_regulator(uint16_t pressure)  //unit 100pa
{
  //_analog_write()
}
void _do_processing()
{
  int32_t pressure = 0;
  _get_pressure(&pressure);
  monitoring.pressure = pressure;

  switch(state.state)
  {
    case 0:  //ready
      state.state = 1;
      monitoring.time = 0;
      monitoring.pressure = 0;
      break;
    case 1:  //low vac.
      monitoring.time = monitoring.time+1;
      
      if(channel.enable[state.channel]==1)
      {      
        //if(monitoring.dispensingflag == 1)
        if(_check_dispensing_on()==1)
        {          
          state.state = 2;
          monitoring.time = 0;
#ifdef DEBUG                 
          Serial.println("MSG: Low Vac. End");
#endif          
        }
        else
        {          
          //low vac. sol control
          //exhaust sol control 
         _set_solenoid_control(0, 0, 0, 1);  // cda off, exhaust off, high vac off, low vac on   
         _set_solenoid_control(0, 0, 0, 0);  // cda off, exhaust off, high vac off, low vac off
         _set_solenoid_control(0, 1, 0, 0);  // cda off, exhaust on, high vac off, low vac off
        }
      }
      else
      {  
        //alarm display      
        monitoring.time = 0;
      }      
      break;
    case 2:  //rising
      monitoring.time++;
      _set_solenoid_control(1, 0, 0, 0);  // cda on, exhaust off, high vac off, low vac off
      if(channel.risingmode[state.channel] == 0)
      {
#ifdef DEBUG               
        Serial.println("MSG: Rising End, "+String(monitoring.time)+" msec");
#endif        
        state.state = 3;      
        monitoring.time = 0;  
        _set_cda_regulator(channel.dispensingpressure[state.channel]);
        _set_vac_regulator(channel.highvacpressure[state.channel]);        
      }
      else
      {
        if(monitoring.time>=channel.risingtime[state.channel])
        {
#ifdef DEBUG                 
          Serial.println("MSG: Rising End, "+String(monitoring.time)+" msec");
#endif          
          state.state = 3;
          monitoring.time = 0;
          _set_cda_regulator(channel.dispensingpressure[state.channel]);
          _set_vac_regulator(channel.highvacpressure[state.channel]);
        }
        else
        {
          //rising control
          uint16_t pressure_cur = channel.dispensingpressure[state.channel]*(monitoring.time/channel.risingtime[state.channel]);
          _set_cda_regulator(pressure_cur);
        }
      }      
      break;
    case 3: //dispensing
      monitoring.time++;
      if(channel.pulsemode[state.channel] == 0)
      {
        if(_check_dispensing_on() == 0)
        //if(monitoring.dispensingflag == 0)
        {
#ifdef DEBUG                 
          Serial.println("MSG: Disp End, "+String(monitoring.time)+" msec");
#endif          
          state.state = 4;
          monitoring.time = 0;          
        }      
      }
      else
      {
        if(monitoring.time>=channel.dispensingtime[state.channel])
        {
#ifdef DEBUG                 
          Serial.println("MSG: Disp End, "+ String(monitoring.time)+" msec");
#endif          
          state.state = 4;
          monitoring.time = 0;          
          monitoring.dispensingflag = 0;
        }
      }
      break;
    case 4: //exhaust
      monitoring.time++;
      _set_solenoid_control(0, 1, 0, 0);  // cda off, exhaust on, high vac off, low vac off
      if(monitoring.time>=channel.exhausttime[state.channel])
      {
#ifdef DEBUG               
        Serial.println("MSG: Exh End, "+String(monitoring.time)+" msec");
#endif        
        state.state = 5;
        monitoring.time = 0;        
      }
      break;
    case 5: //high vac.
      monitoring.time++;  
      _set_solenoid_control(0, 0, 1, 0);  // cda off, exhaust off, high vac on, low vac off
      if(monitoring.time>=channel.highvactime[state.channel])
      {
#ifdef DEBUG       
        Serial.println("MSG: High Vac End, "+String(monitoring.time)+" msec");
#endif           
        state.state = 1;
        monitoring.time = 0;        
      }
  
      break;    
  }
}
