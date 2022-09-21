#pragma once

#include "pinouts.h"
#include "math.h"

const int cNumPixels = 128;
const int cCountStart = 15;
const int cCountEnd = 127;

const int cEffectiveLineWidthMin = 10;
const int cEffectiveLineWidthMax = 30;
const int minWidth = 15, maxWidth = 30;

uint64_t clockCycle = 0;
const int deadZone = 15;

int linearPixelsData[cNumPixels]{};
bool binaryPixelsRawData[cNumPixels]{};
bool binaryPixelsOneHotData[cNumPixels]{};
int trackMidPointStore = -1;

void pinoutInitCCD()
{
    pinMode(PINOUT_CCD_SI, OUTPUT);
    pinMode(PINOUT_CCD_CLK, OUTPUT);
    pinMode(PINOUT_CCD_AO, INPUT);

    digitalWrite(PINOUT_CCD_SI, LOW);  // IDLE state
    digitalWrite(PINOUT_CCD_CLK, LOW); // IDLE state
}

void captrueCCD(int explosureTimeMs = 20)
{
    digitalWrite(PINOUT_CCD_CLK, LOW);
    delayMicroseconds(1);
    digitalWrite(PINOUT_CCD_SI, HIGH);
    delayMicroseconds(1);

    digitalWrite(PINOUT_CCD_CLK, HIGH);
    delayMicroseconds(1);
    digitalWrite(PINOUT_CCD_SI, LOW);
    delayMicroseconds(1);

    digitalWrite(PINOUT_CCD_CLK, LOW);
    delayMicroseconds(2);

    /* and now read the real image */

    for (int i = 0; i < cNumPixels; i++)
    {
        digitalWrite(PINOUT_CCD_CLK, HIGH);

        delayMicroseconds(2);
        linearPixelsData[i] = analogRead(PINOUT_CCD_AO); // 8-bit is enough
        digitalWrite(PINOUT_CCD_CLK, LOW);
        delayMicroseconds(2);
    }

    digitalWrite(PINOUT_CCD_CLK, HIGH);
    delayMicroseconds(2);

    for (int t = 0; t < explosureTimeMs * 250; t++)
    {
        digitalWrite(PINOUT_CCD_CLK, LOW);
        delayMicroseconds(2);

        digitalWrite(PINOUT_CCD_CLK, HIGH);
        delayMicroseconds(2);
    }
}

void printCCDLinearData(int maxVal)
{
    for (int i = 0; i < cNumPixels; i++)
    {
        int t = floor(float(linearPixelsData[i]) / float(maxVal) * 10.0f - 0.1f);
        Serial.print(char(48 + t));
    }
    Serial.println();
}

void printCCDBinaryRawData()
{
    for (int i = 0; i < cNumPixels; i++)
    {
        char c = binaryPixelsRawData[i] ? 'x' : '-';
        Serial.print(c);
    }
    Serial.println();
}

void printCCDOneHotData()
{
    for (int i = 0; i < cNumPixels; i++)
    {
        char c = binaryPixelsOneHotData[i] ? '^' : ' ';
        Serial.print(c);
    }
    Serial.println();
}

void linearToRawBinary(int &minVal, int &maxVal, int &avgVal)
{
    maxVal = 0;
    minVal = 1e6;

    for (int i = cCountStart; i < cCountEnd; i++)
    {
        int currentVal = linearPixelsData[i];

        if (maxVal < currentVal)
            maxVal = currentVal;
        if (minVal > currentVal)
            minVal = currentVal;
    }
    avgVal = customRound(float(minVal + maxVal) / 2.0f);

    for (int i = 0; i < deadZone; i++)
    {
        linearPixelsData[i] = 0;
    }

    for (int i = deadZone; i < cNumPixels; i++)
    {
        binaryPixelsRawData[i] = (linearPixelsData[i] < avgVal) ? true : false;
        avgVal = customRound(float(minVal + maxVal) / 2.0f);

        for (int i = cCountStart; i < cCountEnd; i++)
        {
            binaryPixelsRawData[i] = (linearPixelsData[i] < avgVal) ? true : false;
        }
    }
}

void drawOneHot(int point)
{
    for (int i = 0; i < cNumPixels; i++)
    {
        if (i == point)
        {
            binaryPixelsOneHotData[i] = true;
            continue;
        }
        binaryPixelsOneHotData[i] = false;
    }
}

bool getMidPoint(int fromPixel, int &trackMidPixel, int &trackEndPixel)
{
    int accumulatedDarkPixel = 0;

    int trackLeftPixel = -1;
    int trackRightPixel = -1;
    int trackMidPixelTmp = -1;

    for (int i = fromPixel; i < cCountEnd; i++)
    {
        bool currentPixel = binaryPixelsRawData[i];

        // Dark pixel
        if (currentPixel == true)
        {
            if (trackLeftPixel == -1)
            {
                trackLeftPixel = i;
            }
            accumulatedDarkPixel++;
        }

        // White pixel
        else
        {
            if (accumulatedDarkPixel >= customRound(cEffectiveLineWidthMin) &&
                accumulatedDarkPixel <= customRound(cEffectiveLineWidthMax))
            {
                trackRightPixel = i - 1;
                break;
            }

            accumulatedDarkPixel = 0;
            trackLeftPixel = -1;
        }
    }

    // Parse invalid, retain last array
    if (trackLeftPixel == -1 || trackRightPixel == -1)
        return false;

    // Get mid point
    if ((trackRightPixel - trackLeftPixel) % 2 == 0)
    {
        trackMidPixelTmp = customRound((trackRightPixel + trackLeftPixel) / 2.0f);
    }
    else
    {
        int trackMidPixelCandidate1 = customRound((trackRightPixel + trackLeftPixel - 1) / 2.0f);
        int trackMidPixelCandidate2 = trackMidPixelCandidate1 + 1;

        trackMidPixelTmp =
            (linearPixelsData[trackMidPixelCandidate1] <
             linearPixelsData[trackMidPixelCandidate2])
                ? trackMidPixelCandidate1
                : trackMidPixelCandidate2;
    }

    trackMidPixel = trackMidPixelTmp;
    trackEndPixel = trackRightPixel + 1;
    return true;
}

bool formerOneIsCloserToCenter(int a, int b)
{
    int midPoint = customRound((cCountEnd - cCountStart) / 2.0f);
    return abs(a - midPoint) < abs(b - midPoint);
}

void rawBinaryToOneHot()
{
    int trackMidPixelTmp = -1;
    int trackMidPixel = 0;
    int trackEndPixel = cCountStart;

    while (getMidPoint(trackEndPixel, trackMidPixel, trackEndPixel))
    {
        if (formerOneIsCloserToCenter(trackMidPixel, trackMidPixelTmp))
            trackMidPixelTmp = trackMidPixel;
    }

    trackMidPointStore = trackMidPixelTmp;

    if (trackMidPointStore != -1)
        drawOneHot(trackMidPointStore);
}

int getBias()
{
    int midPoint = cNumPixels / 2;
    int bias = 0;
    for (int i = 0; i < cNumPixels; i++)
    {
        if (binaryPixelsRawData[i])
        {
            bias += (i - midPoint);
        }
    }
    return bias / 8;
}

int getTrackMidPoint()
{
    return customRound(map(float(trackMidPointStore), float(cCountStart), float(cCountEnd), 0.0f, 128.0f));
}

// int getBias()
// {
//     int midPoint = cNumPixels / 2;
//     int bias = 0;
//     for (int i = 0; i < cNumPixels; i++)
//     {
//         if (binaryPixelsRawData[i])
//         {
//             bias += (i - midPoint);
//         }
//     }
//     return bias / 8;
// }

int processCCD()
{
    int minVal = 0;
    int maxVal = 0;
    int avgVal = 0;

    // Capture
    captrueCCD(20);

    // Process
    linearToRawBinary(minVal, maxVal, avgVal);
    rawBinaryToOneHot();

    // Debug
    printCCDLinearData(maxVal);
    printCCDBinaryRawData();
    printCCDOneHotData();

    // Return
    return getTrackMidPoint();
}