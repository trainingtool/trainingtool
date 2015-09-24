// SGS Calibration by linear interpolation for Strain 1 and Strain 2

// Put two known loads on the Strain Gauge sensor and write obtained values below :  (You can use Strain 1 or Strain 2 or the two Strains) 

// variables for interrupt sequence
long hitsWheel = 0;


volatile unsigned long curMicroWheel;


int wheelReadingThreshold = 2;
float rpmTol = 1.5;


unsigned long currentMillis = 0;
unsigned long prevMillis = 0;

unsigned long lastWheelTime = 0;
int readingW = 0;
int prevReadingW=0;
float rpmW=0;


float ReadingA_Strain1 = 301.0;
float LoadA_Strain1 = 0.0; //  (Kg,lbs..) 
float ReadingB_Strain1 = 310.0;
float LoadB_Strain1 = 1.0; //  (Kg,lbs..) 

float ReadingA_Strain2 = 1.0;
float LoadA_Strain2 = 0.0; //  (Kg,lbs..) 
float ReadingB_Strain2 = 60.0;
float LoadB_Strain2 = 80.0; //  (Kg,lbs..) 


int time_step = 10 ; // reading every 2.5s
long time = 0;

void setup() {
  Serial.begin(115200); //  setup serial baudrate
      attachInterrupt(0, countWheel, FALLING);


}

void loop() {
  float newReading_Strain1 = analogRead(0);  // analog in 0 for Strain 1
  float newReading_Strain2 = analogRead(1);  // analog in 1 for Strain 2
  int newReading_hall = digitalRead(2);
  // Calculate load by interpolation 
  float load_Strain1 = ((LoadB_Strain1 - LoadA_Strain1)/(ReadingB_Strain1 - ReadingA_Strain1)) * (newReading_Strain1 - ReadingA_Strain1) + LoadA_Strain1;
  float load_Strain2 = ((LoadB_Strain2 - LoadA_Strain2)/(ReadingB_Strain2 - ReadingA_Strain2)) * (newReading_Strain2 - ReadingA_Strain2) + LoadA_Strain2;
  
  // millis returns the number of milliseconds since the board started the current program
  if(millis() > time_step+time) {
    //Serial.print("Reading_Strain1 : ");
    //Serial.print(newReading_Strain1);     // display strain 1 reading
    //Serial.print("  Load_Strain1 : ");
    Serial.print(time);
        Serial.print(" , ");
    Serial.println(load_Strain1);     // display strain 1 load
    //Serial.print(" RPM: ");
    //Serial.println(rpmW);
//    Serial.print("Reading_Strain2 : ");
//    Serial.print(newReading_Strain2);     // display strain 2 reading
//    Serial.print("  Load_Strain2 : ");
//    Serial.println(load_Strain2);         // display strain 2 load 
//    Serial.println('\n');
    time = millis();
  }
}
//interrupt counters, could also trigger timer measurement here for more accurate RPM
void countWheel()
{
 hitsWheel++;
 if(hitsWheel == wheelReadingThreshold)
 {
     curMicroWheel = micros();
    Serial.println("Wheel hit");
     const int kHolesPerWheel = 1;
     unsigned long dtW = curMicroWheel-lastWheelTime;
     lastWheelTime = curMicroWheel;
     long oldWheelRPM = rpmW;
     rpmW = wheelReadingThreshold*60./(dtW/1000000.);
     rpmW /= kHolesPerWheel;
     hitsWheel = 0;
     
//     if(rpmW/oldWheelRPM>(rpmTol)&&rpmW>=15)
//     {
//        rpmW = oldWheelRPM; 
//     }
     
 }
}
