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
  int cForcePoints; // how many valid points are in rgSpeeds/rgValues?
  int ixUpdatePoint; // what is the next point we're going to update in rgSpeeds/rgValues?
  float flLastSlope;
  float flLastPower;
  float flLastSpeed;
};

// keeps track of the last time we sent each bit of data
struct ReportCounter
{
  const static int POWER_CODE = 0;
  const static int powerInterval = 500;
  long tmLastPower;

  const static int SLOPE_CODE = 3;
  const static int slopeInterval = 5000;
  long tmLastSlope;

  const static int WHEELTURNS_CODE = 4;
  const static int countInterval = 2000;
  long tmLastCount;

  const static int SPEED_CODE = 1;
  const static int speedInterval = 5000;
  long tmLastSpeed;

  const static int POINT_CODE = 2;
  const static int pointInterval = 250;
  long tmLastPoint;
  int ixLastPoint;
};

HistoryData historyCounter;
ReportCounter reportCounter;

int InitHistoryData()
{
  memset(&historyCounter, 0, sizeof(historyCounter));
  memset(&reportCounter, 0, sizeof(reportCounter));
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
  float flNewSlope = CalculateSlope();
  noInterrupts();
  historyCounter.rgSpeeds[historyCounter.ixUpdatePoint] = flWheelSpeed;
  historyCounter.rgValues[historyCounter.ixUpdatePoint] = flValue;
  historyCounter.ixUpdatePoint = (historyCounter.ixUpdatePoint + 1) % HistoryData::kCount;

  historyCounter.cForcePoints = max(historyCounter.cForcePoints, historyCounter.ixUpdatePoint);
  historyCounter.flLastSlope = flNewSlope;
  historyCounter.flLastPower = flValue;
  historyCounter.flLastSpeed = flWheelSpeed;

  interrupts();

}
////////////////////////////////////////////////////
////////////////////////////////////////////////////
////////////////////////////////////////////////////
bool ShouldSendCheck(long* ptmLast, long tmNow, int tmInterval)
{
  if(tmNow > ((*ptmLast) + tmInterval))
  {
    *ptmLast = tmNow;
    return true;
  }
  return false;
}
void HandleDataTx(long tmNow) 
{
  noInterrupts();
  float localLastPower = historyCounter.flLastPower;
  float localLastSlope = historyCounter.flLastSlope;
  float localLastSpeed = historyCounter.flLastSpeed;

  float flPointX = historyCounter.rgSpeeds[reportCounter.ixLastPoint];
  float flPointY = historyCounter.rgValues[reportCounter.ixLastPoint];
  interrupts();
  if(ShouldSendCheck(&reportCounter.tmLastPower, tmNow, reportCounter.powerInterval)) 
  {
    Serial.print(ReportCounter::POWER_CODE);
    Serial.print(":");
    Serial.println(localLastPower);
  }
  if(ShouldSendCheck(&reportCounter.tmLastSlope, tmNow, reportCounter.slopeInterval)) 
  {
    Serial.print(ReportCounter::SLOPE_CODE);
    Serial.print(":");
    Serial.println(localLastSlope);
  }
  if(ShouldSendCheck(&reportCounter.tmLastCount, tmNow, reportCounter.countInterval)) 
  {
    Serial.print(ReportCounter::WHEELTURNS_CODE);
    Serial.print(":");
    Serial.println(0);
  }
  if(ShouldSendCheck(&reportCounter.tmLastSpeed, tmNow, reportCounter.speedInterval)) 
  {
    Serial.print(ReportCounter::SPEED_CODE);
    Serial.print(":");
    Serial.println(localLastSpeed);
  }
  if(historyCounter.cForcePoints > 0 && ShouldSendCheck(&reportCounter.tmLastPoint, tmNow, reportCounter.pointInterval)) 
  {
    reportCounter.ixLastPoint++;
    if(reportCounter.ixLastPoint >= historyCounter.cForcePoints)
    {
      reportCounter.ixLastPoint = 0;
    }
    Serial.print(ReportCounter::POINT_CODE);
    Serial.print(":");
    Serial.print(flPointX);
    Serial.print(",");
    Serial.print(flPointY);
    Serial.print(",");
    Serial.println(reportCounter.ixLastPoint);
  }
}

void setup() {
  delay(2000); // make sure we have a chance to re-upload stuff before this thing starts hammering on the serial port
  
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
  HandleDataTx(tmNow);
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
      
      cRpmsSinceLastStore++;
      flRpmSumSinceLastStore += rpmW;
      
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
