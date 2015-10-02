// SGS Calibration by linear interpolation for Strain 1 and Strain 2

// Put two known loads on the Strain Gauge sensor and write obtained values below :  (You can use Strain 1 or Strain 2 or the two Strains) 
//constants:
const int time_step = 250; // milliseconds between serial outputs
const int kHolesPerWheel = 2;



unsigned long lastWheelTime = 0;

unsigned long cRpmsSinceLastReport = 0;
float flRpmSumSinceLastReport = 0;

unsigned long cStrainsSinceLastReport=0;
float flStrainSumSinceLastReport=0;


unsigned int loops = 0;

long lastPrintTime = 0;

//////////////////////////////
////////////////////////////
// linear regression for speed->power

#ifdef _MSC_VER
typedef int int32;
#else
typedef long long int32;
#endif

struct HistoryData {
  const static int32 kCount = 5;
  float rgSpeeds[kCount];
  float rgValues[kCount];
  int cForcePoints;
  int ixUpdatePoint;
  float flLastSlope;
};

HistoryData historyCounter;

int InitHistoryData()
{
  memset(&historyCounter, 0, sizeof(historyCounter));
}

float CalculateSlope()
{
  float sum_yixi = 0;
  float sum_xi2 = 0;
  for(int x = 0;x < historyCounter.cForcePoints; x++)
  {
    sum_yixi += historyCounter.rgSpeeds[x]*historyCounter.rgValues[x];
    sum_xi2 += historyCounter.rgSpeeds[x]*historyCounter.rgSpeeds[x];
  }

  if(sum_xi2 != 0)
  {
    return sum_yixi / sum_xi2;
  }
  return 0;
}
void HandleDataReport(float flWheelSpeed, float flValue)
{
  historyCounter.rgSpeeds[historyCounter.ixUpdatePoint] = flWheelSpeed;
  historyCounter.rgValues[historyCounter.ixUpdatePoint] = flValue;
  historyCounter.ixUpdatePoint = (historyCounter.ixUpdatePoint + 1) % HistoryData::kCount;

  historyCounter.cForcePoints = max(historyCounter.cForcePoints, historyCounter.ixUpdatePoint);

}
////////////////////////////////////////////////////
////////////////////////////////////////////////////
////////////////////////////////////////////////////


void setup() {
  Serial.begin(115200); //  setup serial baudrate
  attachInterrupt(0, countWheel, FALLING);

  InitHistoryData();
}

float getCurrentStrain()
{
  const float ReadingA_Strain1 = 297.8;
  const float LoadA_Strain1 = 0.0; //  (Kg,lbs..) 
  const float ReadingB_Strain1 = 383.5;
  const float LoadB_Strain1 = 5; //  (Kg,lbs..) 
  const float slopeCalibration =  ((LoadB_Strain1 - LoadA_Strain1)/(ReadingB_Strain1 - ReadingA_Strain1));

  const float newReading_Strain1 = analogRead(0);  // analog in 0 for Strain 1
  // Calculate load by interpolation 
  const float load_Strain1 = slopeCalibration * (newReading_Strain1 - ReadingA_Strain1) + LoadA_Strain1;
  
  return newReading_Strain1;
}

void loop() 
{
  loops++;
  
  flStrainSumSinceLastReport += getCurrentStrain();
  cStrainsSinceLastReport++;
  
  // millis returns the number of milliseconds since the board started the current program
  const long tmNow = millis();
  if(tmNow > time_step+lastPrintTime && cRpmsSinceLastReport > 0) {
    
    // get the RPM, convert RPM to power
    float flRPMNow = flRpmSumSinceLastReport / cRpmsSinceLastReport;
    flRpmSumSinceLastReport = 0;
    cRpmsSinceLastReport = 0;
    
    float flSlopeNow = CalculateSlope();
    float flImpliedPower = flRPMNow * flSlopeNow;
    
    
    Serial.print(loops);
    Serial.print("\t");
    Serial.print(tmNow);
    Serial.print("\t");
    Serial.print(flRPMNow);
    Serial.print("\t");
    Serial.println(flImpliedPower);
    Serial.print("\t");
    Serial.println(flSlopeNow);
    Serial.print("\t");
    Serial.println(historyCounter.cForcePoints);
    Serial.print("\t");
    Serial.println(historyCounter.ixUpdatePoint);
    lastPrintTime = tmNow;
  }
}
void countWheel()
{
    const unsigned long curMicroWheel = micros();
    
    const unsigned long dtW = curMicroWheel-lastWheelTime;
    lastWheelTime = curMicroWheel;
    
    if(dtW > 50000) // >50000 so we don't get spurious readings
    {
      float rpmW = 60.*1000000./((float)dtW);
      rpmW /= kHolesPerWheel;
      
      const float flStrainNowKg = flStrainSumSinceLastReport / cStrainsSinceLastReport;
      cStrainsSinceLastReport = 0;
      flStrainSumSinceLastReport = 0;
      const float flStrainNowN = flStrainNowKg*9.8;
      const float flSpeedNowMs = rpmW / 60 * 0.7*3.14159;
      const float flPowerNow = flStrainNowN*flSpeedNowMs;
      
      
      HandleDataReport(rpmW, flPowerNow);
      
      cRpmsSinceLastReport++;
      flRpmSumSinceLastReport += rpmW;
    }
}
