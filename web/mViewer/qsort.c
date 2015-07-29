/*
This is an alpha version.  
Copyright (c) 1996 California Institute of Technology 
*/
 
#include <stdio.h>
#include <stdlib.h>
#include <math.h>

void sort (double *list, int n)
{
    int     i, l;
    double  temp;

    for (l=1; l<n; l++) {
	
	temp = list[l];

	i = l - 1;
	while ((i >= 0) && (list[i] > temp)) {

	    list[i+1] = list[i];
	    i--;
	    if (i == 0)
		break;
        }

	list[i+1] = temp;
    }
}

void swap (double *p, int *pind, double *q, int *qind)
{
    int   ltmp;
    double temp;
   
    int  debug = 0;


    if (debug) {
        printf ("From swap: p= [%f] pind= [%d] q= [%f] qind= [%d]\n", 
	    *p, *pind, *q, *qind);
        fflush (stdout);
    }


    temp = *p;
    *p = *q;
    *q = temp;
    
    ltmp = *pind;
    *pind = *qind;
    *qind = ltmp;
   
    return;
}


int partition (double *list, int *ind, int n, double pivot_value)
{
    int  i, j, l, ii, jj;
    
    int  debug = 0;

    if (debug) {
        printf ("from partition: n= %d pivot_value= %f\n", n, pivot_value);
        for (l=0; l<n; l++) {
	    printf ("l= %d list= %f\n", l, list[l]);
        }
    }


    i = 0;
    j = n-1;
     
    while (i <= j) {
	
	while (list[i] < pivot_value) {
	    i++;
	    if (i == j)
		break;
	}
   
        if (debug) {
            printf ("xxx1: i= [%d] list= %f\n", i, list[i]);
	    fflush (stdout);
        }

	while (list[j] >= pivot_value) {
	    j--;
	    if (j == i)
		break;
	}

        if (debug) {
	    printf ("xxx2: j= [%d] list= [%f]\n", j, list[j]);
	    fflush (stdout);
        }

/*
	if (i < j) 
	    swap (&list[i++], &list[j--]);
*/

	if (i < j) {
	    ii = i++;
	    jj = j--;

            if (debug) {
                printf ("ii= [%d] jj= [%d]\n", ii, jj);
                printf ("list[ii]= [%f] list[jj]= [%f]\n", list[ii], list[jj]);
	        fflush (stdout);
            }

	    swap (&list[ii], &ind[ii], &list[jj], &ind[jj]);
        }

        if (debug) {
	    printf ("xxx3: i= %d j= %d\n", i, j);
            for (l=0; l<n; l++) {
	        printf ("l= %d list= %f\n", l, list[l]);
            }
	    fflush (stdout);
        }

    }

    if (debug) {
        printf ("i= %d j= %d\n", i, j);
        for (l=0; l<n; l++) {
	    printf ("l= %d list= %f\n", l, list[l]);
        }
        fflush (stdout);
    }

    return (i);
}

int find_pivot (double *list, int n, double *pivot_value)
{
    int     i;
    double  b[3];

    int     debug = 0;

    if (debug) {
        printf ("from find_pivot: n= %d\n", n);
        fflush (stdout); 
    }

    if (n > 3) {
	
	b[0] = list[0];
	b[1] = list[(n-1)/2];
	b[2] = list[n-1];

	sort (b, 3);

	if (b[0] != b[2]) {
	    *pivot_value = (b[0] < b[1]) ? b[1]:b[2];
	    return (1);
	}
    }

    for (i=1; i<n; i++)
	if (list[0] != list[i]) {
	    *pivot_value = (list[0] > list[i]) ? list[0]:list[i];
	    return (1);
	}
    
    return (0);
}

void quicksort (double *list, int *ind, int n)
{
    int     k;
    double  pivot_value;
    
    int     debug = 0;

    if (debug) {
        printf ("quicksort starts: n= %d\n", n);
        fflush (stdout); 
    }

    if (find_pivot(list, n, &pivot_value) != 0) {

	k = partition (list, ind, n, pivot_value);

        if (debug) {
	    printf ("k= %d pivot_value= %f\n", k, pivot_value);
            fflush (stdout); 
        }
       
	quicksort (list, ind, k);
        
	quicksort (&list[k], &ind[k], n-k);
    }
    return;
}


