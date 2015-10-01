// TrainerCode2.cpp : Defines the entry point for the console application.
//

#include "stdafx.h"
#include <string.h>
#include <stdlib.h>
#include <iostream>
using namespace std;


#ifdef _MSC_VER
typedef int int32;
#else
typedef long long int32;
#endif

struct ForcePoint
{
  ForcePoint() : wheelspeed(0),value(0) {};
  ForcePoint(float _wheelspeed, float _value) : wheelspeed(_wheelspeed), value(_value) {};

  float wheelspeed;
  float value;
};
struct HistoryData
{
  HistoryData() : cForcePoints(0), flLastSlope(0), ixUpdatePoint(0) {}

  const static int32 kCount = 5;
  ForcePoint rgForces[kCount];
  int32 cForcePoints;
  int32 ixUpdatePoint;
  float flLastSlope;
};


void InitHistoryData(HistoryData* pData)
{
  memset(pData, 0, sizeof(*pData));
}
float CalculateSlope(const HistoryData* pData)
{
  float sum_yixi = 0;
  float sum_xi2 = 0;
  for(int x = 0;x < pData->cForcePoints; x++)
  {
    const ForcePoint& fp = pData->rgForces[x];
    sum_yixi += fp.value*fp.wheelspeed;
    sum_xi2 += fp.wheelspeed*fp.wheelspeed;
  }

  if(sum_xi2 != 0)
  {
    return sum_yixi / sum_xi2;
  }
  return 0;
}
void HandleDataReport(HistoryData* pData, const ForcePoint& fp)
{
  pData->rgForces[pData->ixUpdatePoint] = fp;
  pData->ixUpdatePoint = (pData->ixUpdatePoint + 1) % HistoryData::kCount;

  pData->cForcePoints = max(pData->cForcePoints, pData->ixUpdatePoint);

  pData->flLastSlope = CalculateSlope(pData);
}

float ZeroToOne()
{
  float flRand = (float)(rand() % 10000);
  return flRand / 10000.0f;
}
float NegOneToOne()
{
  return (ZeroToOne()-0.5f)*2.0f;
}

void GenerateData(float* rpm, float* power)
{
  *rpm = ZeroToOne() * 5 + 200;
  *power = (*rpm)*5 + NegOneToOne()*50; // y=mx+b, m = 5, b = 0
}

int _tmain(int argc, _TCHAR* argv[])
{
  HistoryData hd;
  for(int x = 0; x < 10000; x++)
  {
    float rpm;
    float power;
    GenerateData(&rpm, &power);
    HandleDataReport(&hd,ForcePoint(rpm,power));
  }


  system("pause");
	return 0;
}

