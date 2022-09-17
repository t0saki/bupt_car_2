#pragma once

#include "pinouts.h"
#include "math.h"

const int cNumPixels = 128;
const int cEffectiveLineWidth = 18;
const float cEffectiveLineWidthTolerence = 0.2f;

extern int linearPixelsData[cNumPixels]{};
extern bool binaryPixelsRawData[cNumPixels]{};
extern bool binaryPixelsOneHotData[cNumPixels]{};

void pinoutInitCCD()
{
    pinMode(PINOUT_CCD_SI, OUTPUT);
    pinMode(PINOUT_CCD_CLK, OUTPUT);
    pinMode(PINOUT_CCD_AO, INPUT);

    digitalWrite(PINOUT_CCD_SI, LOW);  // IDLE state
    digitalWrite(PINOUT_CCD_CLK, LOW); // IDLE state
}

void captrueCCD()
{
    delayMicroseconds(1); /* Integration time in microseconds */
    delay(10);            /* Integration time in miliseconds  */

    digitalWrite(PINOUT_CCD_CLK, LOW);
    digitalWrite(PINOUT_CCD_SI, HIGH);
    digitalWrite(PINOUT_CCD_CLK, HIGH);
    digitalWrite(PINOUT_CCD_SI, LOW);

    delayMicroseconds(1);

    /* and now read the real image */

    for (int i = 0; i < cNumPixels; i++)
    {
        linearPixelsData[i] = analogRead(PINOUT_CCD_AO) / 4; // 8-bit is enough
        digitalWrite(PINOUT_CCD_CLK, LOW);
        delayMicroseconds(1);
        digitalWrite(PINOUT_CCD_CLK, HIGH);
    }
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

void printCCDBinaryProcessedData()
{
    for (int i = 0; i < cNumPixels; i++)
    {
        char c = binaryPixelsOneHotData[i] ? '#' : '-';
        Serial.print(c);
    }
    Serial.println();
}

void linearToRawBinary()
{
    int totalVal = 0;

    for (int i = 0; i < cNumPixels; i++)
    {
        totalVal += linearPixelsData[i];
    }

    int avgVal = customRound(float(totalVal) / float(cNumPixels));

    for (int i = 0; i < cNumPixels; i++)
    {
        binaryPixelsRawData[i] = (linearPixelsData[i] < avgVal) ? true : false;
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

    for (int i = fromPixel; i < cNumPixels; i++)
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
            if (
                accumulatedDarkPixel >= customRound(cEffectiveLineWidth * (1 - cEffectiveLineWidthTolerence)) &&
                accumulatedDarkPixel <= customRound(cEffectiveLineWidth * (1 + cEffectiveLineWidthTolerence)))
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
    int midPoint = customRound(cNumPixels / 2.0f);

    return abs(a - midPoint) < abs(b - midPoint);
}

void rawBinaryToOneHot()
{
    int trackMidPixelFinal = -1;
    int trackMidPixel = 0;
    int trackEndPixel = 0;

    while (getMidPoint(trackEndPixel, trackMidPixel, trackEndPixel))
    {
        if (formerOneIsCloserToCenter(trackMidPixel, trackMidPixelFinal))
            trackMidPixelFinal = trackMidPixel;
    }

    if (trackMidPixelFinal != -1)
        drawOneHot(trackMidPixelFinal);
}

void processCCD()
{
    // Capture
    captrueCCD();

    // Process
    linearToRawBinary();
    rawBinaryToOneHot();

    // Debug
    // printCCDBinaryRawData();
    // printCCDBinaryProcessedData();
}