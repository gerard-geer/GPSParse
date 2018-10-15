#include <stdlib.h>
/**
 * File: parsegps.h
 * Author: Gerard Geer (gdg160030)
 * Provides several GPS related functions.
 */

/**
 * Updates the GPS state with a GPS sentence provided by either
 * the tests or the GPS module.
 * @param str A pointer to the head of the sentence.
 * @param len The length of the sentence.
 */
void GPS_update(char *str, int len);

/**
 * Returns the most recent speed given to the GPS.
 * @return The most recent speed given to the GPS.
 */
double GPS_getCurrentSpeed(void);

/**
 * Returns the most recent theta given to the GPS.
 * @return The most recent theta given to the GPS.
 */
double GPS_getCurrentTheta(void);

/**
 * Returns the most recent longitude given to the GPS.
 * @return The most recent longitude given to the GPS.
 */
double GPS_getCurrentLongitude(void);

/**
 * Returns the most recent latitude given to the GPS.
 * @return The most recent latitude given to the GPS.
 */
double GPS_getCurrentLatitude(void);
