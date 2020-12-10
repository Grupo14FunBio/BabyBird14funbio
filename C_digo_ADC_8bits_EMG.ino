//Variable indicadora de recorte (frecuenciómetro)
boolean clipping = 0;

//Variables que almacenan datos
byte newData = 0;
byte prevData = 0;
unsigned int time = 0;//keeps time and sends vales to store in timer[] occasionally
int timer[10];//sstorage for timing of events
int slope[10];//storage fro slope of events
unsigned int totalTimer;//used to calculate period
unsigned int period;//storage for period of wave
byte index = 0;//current storage index
float frequency;//storage for frequency calculations
int maxSlope = 0;//used to calculate max slope as trigger point
int newSlope;//storage for incoming slope data
byte maxVoltage=0;
float maxVoltage_presentation;
float risk;
float difFreq;
float difVol;
int risk_bt;

//Variables del frecuenciómetro
byte noMatch = 0;//counts how many non-matches you've received to reset variables if it's been too long
byte slopeTol = 3;//slope tolerance- adjust this if you need
int timerTol = 10;//timer tolerance- adjust this if you need


void setup(){
  delay(5000);                                                        //IMPORTANTE
  Serial.begin(9600);
  
  pinMode(13,OUTPUT);//led indicator pin
  pinMode(12,OUTPUT);//output pin
  
  cli();//diable interrupts
  
  //set up continuous sampling of analog pin 0 at 38.5kHz
 
  //clear ADCSRA and ADCSRB registers
  ADCSRA = 0;
  ADCSRB = 0;
  
  ADMUX |= (1 << REFS0); //set reference voltage
  ADMUX |= (1 << ADLAR); //left align the ADC value- so we can read highest 8 bits from ADCH register only
  
  ADCSRA |= (1 << ADPS2) | (1 << ADPS0); //set ADC clock with 32 prescaler- 16mHz/32=500kHz
  ADCSRA |= (1 << ADATE); //enabble auto trigger
  ADCSRA |= (1 << ADIE); //enable interrupts when measurement complete
  ADCSRA |= (1 << ADEN); //enable ADC
  ADCSRA |= (1 << ADSC); //start ADC measurements
  
  sei();//enable interrupts
}

ISR(ADC_vect) {//when new ADC value ready
  
  PORTB &= B11101111;//set pin 12 low
  prevData = newData;//store previous value
  newData = ADCH;//get value from A0
if (newData > maxVoltage && timer){                                                       //IMPORTANTE
    maxVoltage = newData;
    maxVoltage_presentation = maxVoltage * (2.52 * 2 / 256.0);
  }
  if (prevData < 64 && newData >=64){//if increasing and crossing midpoint                //IMPORTANTE
    newSlope = newData - prevData;//calculate slope
    if (abs(newSlope-maxSlope)<slopeTol){//if slopes are ==
      //record new data and reset time
      slope[index] = newSlope;
      timer[index] = time;
      time = 0;
      if (index == 0){//new max slope just reset
        PORTB |= B00010000;//set pin 12 high
        noMatch = 0;
        index++;//increment index
      }
      else if (abs(timer[0]-timer[index])<timerTol && abs(slope[0]-newSlope)<slopeTol){//if timer duration and slopes match
        //sum timer values
        totalTimer = 0;
        for (byte i=0;i<index;i++){
          totalTimer+=timer[i];
        }
        period = totalTimer;//set period
        //reset new zero index values to compare with
        timer[0] = timer[index];
        slope[0] = slope[index];
        index = 1;//set index to 1
        PORTB |= B00010000;//set pin 12 high
        noMatch = 0;
      }
      else{//crossing midpoint but not match
        index++;//increment index
        if (index > 9){
          reset();
        }
      }
    }
    else if (newSlope>maxSlope){//if new slope is much larger than max slope
      maxSlope = newSlope;
      time = 0;//reset clock
      noMatch = 0;
      index = 0;//reset index
    }
    else{//slope not steep enough
      noMatch++;//increment no match counter
      if (noMatch>9){
        reset();
      }
    }
  }
    
  if (newData == 0 || newData == 1023){//if clipping
    //PORTB |= B00100000;//set pin 13 high- turn on clipping indicator led                /YA NO PRENDE EL LED
    clipping = 1;//currently clipping
  }
  
  time++;//increment timer at rate of 38.5kHz
}

void reset(){//clea out some variables
  index = 0;//reset index
  noMatch = 0;//reset match couner
  maxSlope = 0;//reset slope
}



//YA NO USA ESTA FUNCIÓN
void checkClipping(){//manage clipping indicator LED
  if (clipping){//if currently clipping
    PORTB &= B11011111;//turn off clipping indicator led
    clipping = 0;
  }
}


void loop(){
  
  //checkClipping();
  
  frequency = 38462/float(period);//calculate frequency timer rate/period
  
  //print results
  if(frequency < 123132564456){
    /*Serial.print(frequency);
    Serial.println(" hz");
    Serial.print(maxVoltage_presentation);
    Serial.println(" V");*/
    difFreq=frequency-0.56;
    difVol=maxVoltage_presentation-1.42;
    if(difVol>=0 && difFreq>=0){
      risk = 50 + ((difFreq*50+difVol*50)/2);
      /*Serial.print(risk);
      Serial.println(" %");*/
      //if(Serial.available()>0){
      risk_bt=(int)risk;          //Casteamos a una variable entera porque Kodular solo permite valores enteros
      Serial.println(risk_bt);
      //}
      PORTB |= B00100000;//set pin 13 high- turn on clipping indicator led
    }
  }

  delay(100);//feel free to remove this if you want
  
  //do other stuff here
}
