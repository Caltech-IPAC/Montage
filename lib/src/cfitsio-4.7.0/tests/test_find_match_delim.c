/*
 * Tests for fits_find_match_delim - delimiter matching.
 */

#include "fitsio2.h"
#include "test_macros.h"

int
main(void)
{
	fail_if(*fits_find_match_delim("'Xaa'aaa", '\'') != 'X');
	fail_if(*fits_find_match_delim("aaaa'Xaa", '\'') != 'X');
	fail_if(*fits_find_match_delim("aaaaaaa'", '\'') != '\0');

	fail_if(*fits_find_match_delim("\"X", '"') != 'X');
	fail_if(*fits_find_match_delim("x\"X", '"') != 'X');
	fail_if(*fits_find_match_delim("\"", '"') != '\0');

	fail_if(*fits_find_match_delim("}X", '}') != 'X');
	fail_if(*fits_find_match_delim(" {}  }X", '}') != 'X');

	fail_if(*fits_find_match_delim("]X", ']') != 'X');
	fail_if(*fits_find_match_delim(" []  ]X", ']') != 'X');

	fail_if(*fits_find_match_delim(")X", ')') != 'X');
	fail_if(*fits_find_match_delim(" ()  )X", ')') != 'X');
}
