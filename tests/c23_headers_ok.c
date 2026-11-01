/* Compile-only check (tests/c23_headers.sh): the public coord and svc
 * APIs can be called with their real arguments from strict C23 code, where
 * an empty parameter list `f()` means `f(void)`. */

#include <stdio.h>
#include <coord.h>
#include <svc.h>

void use_coord(void)
{
   struct COORD from, to;
   double lon, lat, ra, dec, s;
   int neg, h, m;

   convertCoordinates(EQUJ, 2000., 10., 20., GAL, 2000., &lon, &lat, 0.0);
   (void) ccalc(&from, &to, "s", "s");
   (void) sexToDegree("10:00:00", "+20:00:00", &ra, &dec);
   (void) degreeToHMS(ra, 2, &neg, &h, &m, &s);
   (void) degreeToDMS(dec, 2, &neg, &h, &m, &s);
   (void) roundValue(s, 2);
}

void use_svc(void)
{
   (void) svc_value("key");
   (void) svc_run("cmd");
   svc_sighandler(2);
}
