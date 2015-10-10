// SGS Calibration by linear interpolation for Strain 1 and Strain 2

// Put two known loads on the Strain Gauge sensor and write obtained values below :  (You can use Strain 1 or Strain 2 or the two Strains) 
//constants:
const int time_step = 250; // milliseconds between serial outputs
const int kHolesPerWheel = 2;



unsigned long lastWheelTime = 0;

unsigned long cRpmsSinceLastStore = 0;
float flRpmSumSinceLastStore = 0;

unsigned long cRpmsSinceLastPrint = 0;
float flRpmSumSinceLastPrint = 0;

unsigned long cStrainsSinceLastStore=0;
float flStrainSumSinceLastStore=0;


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
  const static int32 kCount = 100;
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
  
  return load_Strain1;
}

void loop() 
{
  loops++;
  
  float flCurrentStrain = getCurrentStrain();
  
  {
    noInterrupts();
    flStrainSumSinceLastStore += flCurrentStrain;
    cStrainsSinceLastStore++;
    interrupts();
  }
  
  // millis returns the number of milliseconds since the board started the current program
  const long tmNow = millis();
  if(tmNow > time_step+lastPrintTime && cRpmsSinceLastPrint > 0) {
    
    // get the RPM, convert RPM to power
    float flRPMNow = flRpmSumSinceLastPrint / cRpmsSinceLastPrint;
    flRpmSumSinceLastPrint = 0;
    cRpmsSinceLastPrint = 0;
    
    float flSlopeNow = CalculateSlope();
    float flImpliedPower = flRPMNow * flSlopeNow;
    
    
    Serial.print(loops);
    Serial.print("\t");
    Serial.print(tmNow);
    Serial.print("\t");
    Serial.print(flRPMNow);
    Serial.print("\t");
    Serial.print(flImpliedPower);
    Serial.print("\t");
    Serial.print(flSlopeNow);
    Serial.print("\t");
    Serial.print(historyCounter.cForcePoints);
    Serial.print("\t");
    Serial.println(historyCounter.ixUpdatePoint);
    
    if(historyCounter.ixUpdatePoint == 99)
    {
      for(int x = 0;x < historyCounter.ixUpdatePoint; x++)
      {
        Serial.print(historyCounter.rgSpeeds[x]);
        Serial.print("\t");
        Serial.println(historyCounter.rgValues[x]);
      }
      historyCounter.ixUpdatePoint = 0;
    }
    
    
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
      
      {
        noInterrupts();
        cRpmsSinceLastPrint++;
        flRpmSumSinceLastPrint += rpmW;
        
        cRpmsSinceLastStore++;
        flRpmSumSinceLastStore += rpmW;
        interrupts();
      }
      
      if(cRpmsSinceLastStore > 10)
      {
        const float flRpmNow = flRpmSumSinceLastStore / cRpmsSinceLastStore;
        cRpmsSinceLastStore = 0;
        flRpmSumSinceLastStore = 0;
        
        float flStrainNowKg=0;
        {
          noInterrupts();
          flStrainNowKg = flStrainSumSinceLastStore / cStrainsSinceLastStore;
          cStrainsSinceLastStore = 0;
          flStrainSumSinceLastStore = 0;
          interrupts();
        }
        const float flStrainNowN = flStrainNowKg*9.8;
        const float flSpeedNowMs = flRpmNow / 60 * 0.7*3.14159;
        const float flPowerNow = flStrainNowN*flSpeedNowMs;
        
        HandleDataReport(flRpmNow, flPowerNow);
      }
    }
    
}
