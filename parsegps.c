#include "parsegps.h"

// Integerized ID strings.
#define PBOD 0x50424f44
#define PBWC 0x50425743
#define PGGA 0x50474741
#define PGLL 0x50474c4c
#define PGSA 0x50475341
#define PGSV 0x50475356
#define PHDT 0x50484454
#define PR00 0x50523030
#define PRMA 0x50524d41
#define PRMB 0x50524d42
#define PRMC 0x50524d43
#define PRTE 0x50525445
#define PTRF 0x50545246
#define PSTN 0x5053544e
#define PVBW 0x50564257
#define PVTG 0x50565447
#define PWPL 0x5057504c
#define PXTE 0x50585445
#define PZDA 0x505a4441

// Other useful definitions.
#define KNOTS_TO_MPH 1.15078
#define MIN_TO_DEG 0.01666667

// Can make this faster by working per int than per char.
typedef struct state
{
    double speed;
    double theta;
    double latDegrees;
    double longDegrees;

} GPS_STATE;

// THE INTERNAL STATE.
volatile GPS_STATE internal_state;

void GPS_parseIntoNParts(char **parts, int n, char *str, int len)
{
    // A description of what we're doing here:
    // We take a string that looks like this:
    // AAA,BBBBB,C,DD, ... ,NN,O,P,QQ
    // (A comma delimited string)
    // and replacing its commas with null characters to terminate each section:
    // AAA BBBBB C DD  ...  NN O P QQ
    // and storing the pointers to the first (and possibly only) character in
    // the segment:
    // AAA BBBBB C DD  ...  NN O P QQ
    // ^   ^     ^ ^        ^  ^ ^ ^
    // Empty sections will point to a null character, which is safe in
    // all atoX functions.

    // The index of the current part.
    int curPart = 0;

    // The first part begins at the beginning of the string.
    parts[curPart++] = str;

    // Now we go through the string and if we come across a comma, we
    // replace it with a null char to terminate the current part,
    // then we increment to the next part and set its beginning to the
    // char after the null.
    for ( int i = 0; i < len && curPart < n; ++i )
    {
        if ( str[i] == ',' )
        {
            str[i] = '\0';
            parts[curPart++] = str+i+1;
        }
    }
}

double GPS_parseCoord(char *seg)
{
    // Here's the format:
    //    XXXYY.ZZ
    // Where:
    // XXX = whole degrees
    // YY = whole minutes
    // .ZZ = fractions of a minute

    // First we have to find that decimal.
    int i = 0;
    char x = seg[i];
    while( x != '\0' && x != '.')
    {
        ++i;
        x = seg[i];
    }

    // If we don't find it, we just bail out and return.
    if ( x == '\0' ) return;

    // Okay so first we can get the minutes.
    double minutes = atof(seg+i-2);

    // Now we do the other part. Note we need to squash the first
    // character of the minutes to give an end to the subsegment
    // containing the degrees. 
    char old = seg[i-2];
    seg[i-2] = '\0';
    double degrees = atof(seg);
    seg[i-2] = old;

    // Now all that's left is to do some simple math and convert this to
    // degrees proper.
    degrees += minutes*MIN_TO_DEG;
    return degrees
}

/**
    $GPRMC,220516,A,5133.82,N,00042.24,W,173.8,231.8,130694,004.2,W*70
             0    1     2   3     4    5    6    7     8      9   10 11


      0   220516     Time Stamp
      1   A          validity - A-ok, V-invalid
      2   5133.82    current Latitude
      3   N          North/South
      4   00042.24   current Longitude
      5   W          East/West
      6   173.8      Speed in knots
      7   231.8      True course
      8   130694     Date Stamp
      9   004.2      Variation
      10  W          East/West
      11  *70        checksum
*/
void GPS_parseGPRMC(GPS_STATE *state, char *str, int len)
{
    // So we need term 2, 4, 6 and 7.
    // TODO: do we need 9 and 10?
    // This means we need to split the string into 8 parts.
    char *parts[8];
    GPS_parseIntoNParts(parts, 9, str, len);

    // Now that we have all the information, we can update
    // the internal state.
    state->theta = atof(parts[7]);
    state->speed = atof(parts[6]) * KNOTS_TO_MPH;

    // Now we just need to parse some coordinates.
    state->latDegrees = GPS_parseCoord(parts[2]);
    state->longDegrees = GPS_parseCoord(parts[4]);
}

/**
    eg3. $GPVTG,t,T, , ,s.ss,N,s.ss,K*hh
                0 1 2 3   4  5  6    7
    0    = Track made good
    1    = Fixed text 'T' indicates that track made good is relative to true north
    2    = not used
    3    = not used
    4    = Speed over ground in knots
    5    = Fixed text 'N' indicates that speed over ground in in knots
    6    = Speed over ground in kilometers/hour
    7    = Fixed text 'K' indicates that speed over ground is in kilometers/hour
    8    = Checksum
*/
void GPS_parseGPVTG(GPS_STATE *state, char *str, int len)
{
    // Looks like we're going to need terms 0 and 4. 6 parts, including discarded
    // then.
    char *parts[6];
    GPS_parseIntoNParts(parts, 6, str, len);

    // With that, we can store all the new data.
    state->theta = atof(parts[0]);
    state->speed = atof(parts[4]) * KNOTS_TO_MPH;
}

/**
    Heading, True.

    Actual vessel heading in degrees Ture produced by any device 
    or system producing true heading.

    $GPHDT,x.x,T
    x.x = Heading, degrees True 
*/
void GPS_parseGPHDT(GPS_STATE *state, char *str, int len)
{
    // Okay this requires us to split it into two parts: our data
    // and the rest of the message.
    char *parts[2];
    GPS_parseIntoNParts(parts, 2, str, len);

    state->theta = atof(parts[0]);
}

void GPS_update(char *str, int len)
{

    // Quick sanitization.
    if ( len < 7 ) return;

    // What's going on here? We take the four characters starting at
    // str[2] and concatenate them into an unsigned integer.
    // This allows us to do a single comparison to determine sentence
    // type rather than 4.
    unsigned int type = (unsigned int) str[2];

    // Let's speed up parsing by trimming the string past the sentence type
    // and first comma.
    str += 7;
    len -= 7;

    switch (type)
    {
        case PRMC: GPS_parseGPRMC(&internal_state, str, len); break;
        case PVTG: GPS_parseGPVTG(&internal_state, str, len); break;
        case PHDT: GPS_parseGPHDT(&internal_state, str, len); break;
        default: break;
    }
}

double GPS_getCurrentSpeed(void)
{
    return internal_state.speed;
}

double GPS_getCurrentTheta(void)
{
    return internal_state.theta;
}

double GPS_getCurrentLongitude(void)
{
    return internal_state.longDegrees;
}
double GPS_getCurrentLatitude(void)
{
    return internal_state.latDegrees;
}