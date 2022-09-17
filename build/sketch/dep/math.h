#pragma once

template <class T>
inline T clamp(T &val, T minimum, T maximum)
{
    if (val < minimum)
    {
        val = minimum;
        return minimum;
    }
    if (val > maximum)
    {
        val = maximum;
        return maximum;
    }
    return val;
}

inline int customRound(float val)
{
    if (val - int(val) > 0.5f)
    {
        return int(val) + 1;
    }
    else
    {
        return int(val);
    }
}

template <class T>
inline T lerp(T &val, T maximum)
{
    clamp(val, 0, 255);

    val = customRound(float(val) * float(maximum) / 255.0f);
    return val;
}

template <class T>
inline T map(T &val, T valMin, T valMax, T outMin, T outMax)
{
    val = T(float(val - valMin) / float(valMax - valMin) * float(outMax - outMin) + outMin);
    return val;
}