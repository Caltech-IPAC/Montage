/* Must NOT compile (tests/c23_headers.sh): convertCoordinates takes nine
 * arguments. With the old K&R declaration this compiled silently, which is
 * how mHPXCoord2Tile passed eight. */

#include <coord.h>

void wrong_argument_count(void)
{
   double lon, lat;

   convertCoordinates(EQUJ, 2000., 10., 20., GAL, 2000., &lon, &lat);
}
