// SGS Calibration by linear interpolation for Strain 1 and Strain 2

// Put two known loads on the Strain Gauge sensor and write obtained values below :  (You can use Strain 1 or Strain 2 or the two Strains) 
//constants:
const int kHolesPerWheel = 2;




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
  float flLastStrain;
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

  const static int STRAIN_CODE = 5;
  const static int strainInterval = 750;
  long tmLastStrain;
  bool fSendingStrain; // sending strain is optional
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
  historyCounter.rgSpeeds[historyCounter.ixUpdatePoint] = flWheelSpeed;
  historyCounter.rgValues[historyCounter.ixUpdatePoint] = flValue;
  historyCounter.ixUpdatePoint = (historyCounter.ixUpdatePoint + 1) % HistoryData::kCount;

  historyCounter.cForcePoints = max(historyCounter.cForcePoints, historyCounter.ixUpdatePoint);
  historyCounter.flLastSlope = flNewSlope;
  historyCounter.flLastPower = flValue;
  historyCounter.flLastSpeed = flWheelSpeed;
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
  if(ShouldSendCheck(&reportCounter.tmLastPower, tmNow, reportCounter.powerInterval)) 
  {
    Serial.print(ReportCounter::POWER_CODE);
    Serial.print(":");
    Serial.println(historyCounter.flLastPower);
  }
  if(ShouldSendCheck(&reportCounter.tmLastSlope, tmNow, reportCounter.slopeInterval)) 
  {
    Serial.print(ReportCounter::SLOPE_CODE);
    Serial.print(":");
    Serial.println(historyCounter.flLastSlope);
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
    Serial.println(historyCounter.flLastSpeed);
  }
  if(reportCounter.fSendingStrain && ShouldSendCheck(&reportCounter.tmLastStrain, tmNow, reportCounter.strainInterval)) 
  {
    Serial.print(ReportCounter::STRAIN_CODE);
    Serial.print(":");
    Serial.println(historyCounter.flLastStrain, 6);
  }
  if(historyCounter.cForcePoints > 0 && ShouldSendCheck(&reportCounter.tmLastPoint, tmNow, reportCounter.pointInterval)) 
  {
    reportCounter.ixLastPoint++;
    if(reportCounter.ixLastPoint >= historyCounter.cForcePoints)
    {
      reportCounter.ixLastPoint = 0;
    }
    const float flPointX = historyCounter.rgSpeeds[reportCounter.ixLastPoint];
    const float flPointY = historyCounter.rgValues[reportCounter.ixLastPoint];
    
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

float getCurrentStrainKg()
{
  const float ReadingA_Strain1 = 339.0; // analog reading when loaded to some fixed weight
  const float LoadA_Strain1 = 0.0; // kg when loaded to ReadingA_Strain1
  const float ReadingB_Strain1 = 449.0; // analog reading when loaded to another fixed weih]ght
  const float LoadB_Strain1 = 1.351; // kg when loaded to ReadingB_Strain1
  const float slopeCalibration =  ((LoadB_Strain1 - LoadA_Strain1)/(ReadingB_Strain1 - ReadingA_Strain1));

  const float newReading_Strain1 = analogRead(0);  // analog in 0 for Strain 1
  historyCounter.flLastStrain = newReading_Strain1;
  // Calculate load by interpolation 
  const float load_Strain1 = slopeCalibration * (newReading_Strain1 - ReadingA_Strain1) + LoadA_Strain1;
  
  return load_Strain1;
}


// logic keeping track of wheel hits
unsigned long g_tmLastWheelHit=0; // microtime of the last wheel hit
unsigned int g_cWheelHits=0; // how many wheels hits have gone by since our last check?

unsigned long g_lastWheelTime = 0;
void processWheelHit(int cWheelHits, const unsigned long curMicroWheel)
{
  const unsigned long tmSinceLastProcess = curMicroWheel-g_lastWheelTime;
  
  if(tmSinceLastProcess > 50000 && cStrainsSinceLastStore > 0) // >50000 so we don't get spurious readings, cStrains so we don't do this report unless we also have strain data
  {
    float rpmW = cWheelHits*60.*1000000./((float)tmSinceLastProcess);
    rpmW /= kHolesPerWheel;
    
    float flStrainNowKg=0;
    {
      flStrainNowKg = flStrainSumSinceLastStore / cStrainsSinceLastStore;
      cStrainsSinceLastStore = 0;
      flStrainSumSinceLastStore = 0;
    }
    const float flStrainNowN = flStrainNowKg*9.8;
    const float flSpeedNowMs = rpmW / 60 * 0.7*3.14159;
    const float flPowerNow = flStrainNowN*flSpeedNowMs;
      
     
    HandleDataReport(rpmW, flPowerNow);
    g_lastWheelTime = curMicroWheel;
  }
}

bool Rx_WaitForByte(byte target)
{
  while(Serial.available())
  {
    byte check = Serial.read();
    if(check == target)
    {
      return true; // sucess!
    }
  }
  return false; // we ran out of bytes without finding the target
}

const static byte SET_SEND_STRAIN = 1; // param is 1 byte, indicating whether we want to send strain
void Rx_HandleVerb(byte verb, byte param)
{
  switch(verb)
  {
    case SET_SEND_STRAIN:
      reportCounter.fSendingStrain = param;
      break;
  }
}
void HandleDataRx()
{
  // from-phone data transmission is:
  // check1: 0xfe
  // check2: 0xef
  // verb: 0x__ (the "verb" we want to do)
  // param: 0x__ (a byte of parameter for that verb.   I guess there could be more byte, verb-dependent)
  const static byte bCheck1 = 0xfe;
  const static byte bCheck2 = 0xef;
  
  int cAvailable = Serial.available();
  while(cAvailable >= 4)
  {
    if(Rx_WaitForByte(bCheck1) && Rx_WaitForByte(bCheck2) && Serial.available() >= 2)
    {
      // we found the check bytes and there's still data left!
      byte verb = Serial.read();
      byte param = Serial.read();
      Rx_HandleVerb(verb, param);
    }

    
    cAvailable = Serial.available();
  }
}

void loop() 
{
  loops++;
  
  float flCurrentStrain = getCurrentStrainKg();
  flStrainSumSinceLastStore += flCurrentStrain;
  cStrainsSinceLastStore++;

  
  {
    noInterrupts();
    unsigned long tmLastWheelHit = g_tmLastWheelHit;
    int cWheelHitsToProcess = g_cWheelHits;
    g_cWheelHits = 0; // clear out the count of wheel hits that have gone by
    interrupts();
    
    if(cWheelHitsToProcess > 0)
    {
      processWheelHit(cWheelHitsToProcess, tmLastWheelHit);
    }
  }

  HandleDataRx();
  
  // millis returns the number of milliseconds since the board started the current program
  const long tmNow = millis();
  HandleDataTx(tmNow);
}
void countWheel()
{
  noInterrupts();
  g_tmLastWheelHit = micros();
  g_cWheelHits++;
  interrupts();
}
