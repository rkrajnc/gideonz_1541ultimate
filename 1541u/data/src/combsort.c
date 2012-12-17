/*
 *   1541 Ultimate - The Storage Solution for your Commodore computer.
 *   Copyright (C) 2009  Gideon Zweijtzer - Gideon's Logic Architectures
 *
 *   This program is free software: you can redistribute it and/or modify
 *   it under the terms of the GNU General Public License as published by
 *   the Free Software Foundation, either version 3 of the License, or
 *   (at your option) any later version.
 *
 *   This program is distributed in the hope that it will be useful,
 *   but WITHOUT ANY WARRANTY; without even the implied warranty of
 *   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 *   GNU General Public License for more details.
 *
 *   You should have received a copy of the GNU General Public License
 *   along with this program.  If not, see <http://www.gnu.org/licenses/>.
 *
 *   Abstract:
 *	 Combsort - Simple, yet efficient sort routine.
 *
 */

#include <types.h>

#pragma codeseg ("FILESYS")

/*
char mycomp(int a, int b)
{
    if(a > b)
        return 1;
    return 0;
}
*/
//static char (*comp)(int,int); //= (char(*)(int,int))haha;
//static void (*swp)(int,int);  //= *swap;

void combsort11(char (*compare)(int,int), void (*swap)(int,int), int size)
{
    static BYTE swaps;
    static int i;
    static int gap;

    gap = size;
//    comp = compare;
//    swp  = swap;
    
    do {
        //update the gap value for a next comb
        if (gap > 1) {
            gap = (10 * gap)/13;
            if ((gap == 10) || (gap == 9))
                gap = 11;
        }
        
        for(i=0,swaps=0;(i+gap)<size;i++) {
            if (compare(i, i+gap) > 0) {
                swap(i, i+gap);
                swaps = 1;
            }
        }
    } while(swaps || (gap > 1));
}
