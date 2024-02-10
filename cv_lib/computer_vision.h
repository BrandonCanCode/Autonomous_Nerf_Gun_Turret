/* computer_vision.h
 *
*/


/* Finds and outputs angles for the targets position.
 *
 * Parameters:
 * time_ms      : in    : Time for predicting where the target may be in milliseconds.
 * horz_deg     : out   : Target's horizontal position in degrees.
 * vert_deg     : out   : Target's vertical position in degrees.
*/
int GetTargetAngles(int time_ms, double *horz_deg, double *vert_deg);
