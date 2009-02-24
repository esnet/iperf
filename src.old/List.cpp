
/*--------------------------------------------------------------- 
 * Copyright (c) 1999,2000,2001,2002,2003                              
 * The Board of Trustees of the University of Illinois            
 * All Rights Reserved.                                           
 *--------------------------------------------------------------- 
 * Permission is hereby granted, free of charge, to any person    
 * obtaining a copy of this software (Iperf) and associated       
 * documentation files (the "Software"), to deal in the Software  
 * without restriction, including without limitation the          
 * rights to use, copy, modify, merge, publish, distribute,        
 * sublicense, and/or sell copies of the Software, and to permit     
 * persons to whom the Software is furnished to do
 * so, subject to the following conditions: 
 *
 *     
 * Redistributions of source code must retain the above 
 * copyright notice, this list of conditions and 
 * the following disclaimers. 
 *
 *     
 * Redistributions in binary form must reproduce the above 
 * copyright notice, this list of conditions and the following 
 * disclaimers in the documentation and/or other materials 
 * provided with the distribution. 
 * 
 *     
 * Neither the names of the University of Illinois, NCSA, 
 * nor the names of its contributors may be used to endorse 
 * or promote products derived from this Software without
 * specific prior written permission. 
 * 
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, 
 * EXPRESS OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES 
 * OF MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE AND 
 * NONINFRINGEMENT. IN NO EVENT SHALL THE CONTIBUTORS OR COPYRIGHT 
 * HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER LIABILITY, 
 * WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, 
 * ARISING FROM, OUT OF OR IN CONNECTION WITH THE
 * SOFTWARE OR THE USE OR OTHER DEALINGS IN THE SOFTWARE. 
 * ________________________________________________________________
 * National Laboratory for Applied Network Research 
 * National Center for Supercomputing Applications 
 * University of Illinois at Urbana-Champaign 
 * http://www.ncsa.uiuc.edu
 * ________________________________________________________________ 
 *
 * List.cpp
 * by Kevin Gibbs <kgibbs@ncsa.uiuc.edu> 
 * ------------------------------------------------------------------- 
 */

#include "List.h"
#include "Mutex.h"
#include "SocketAddr.h"

/*
 * Global List and Mutex variables
 */
Iperf_ListEntry *clients = NULL;
Mutex clients_mutex; 

/*
 * Add Entry add to the List
 */
void Iperf_pushback ( Iperf_ListEntry *add, Iperf_ListEntry **root ) {
    add->next = *root;
    *root = add;
}

/*
 * Delete Entry del from the List
 */
void Iperf_delete ( iperf_sockaddr *del, Iperf_ListEntry **root ) {
    Iperf_ListEntry *temp = Iperf_present( del, *root );
    if ( temp != NULL ) {
        if ( temp == *root ) {
            *root = (*root)->next;
        } else {
            Iperf_ListEntry *itr = *root;
            while ( itr->next != NULL ) {
                if ( itr->next == temp ) {
                    itr->next = itr->next->next;
                    break;
                }
                itr = itr->next;
            }
        }
        delete temp;
    }
}

/*
 * Destroy the List (cleanup function)
 */
void Iperf_destroy ( Iperf_ListEntry **root ) {
    Iperf_ListEntry *itr1 = *root, *itr2;
    while ( itr1 != NULL ) {
        itr2 = itr1->next;
        delete itr1;
        itr1 = itr2;
    }
    *root = NULL;
}

/*
 * Check if the exact Entry find is present
 */
Iperf_ListEntry* Iperf_present ( iperf_sockaddr *find, Iperf_ListEntry *root ) {
    Iperf_ListEntry *itr = root;
    while ( itr != NULL ) {
        if ( SockAddr_are_Equal( (sockaddr*)itr, (sockaddr*)find ) ) {
            return itr;
        }
        itr = itr->next;
    }
    return NULL;
}

/*
 * Check if a Entry find is in the List or if any
 * Entry exists that has the same host as the 
 * Entry find
 */
Iperf_ListEntry* Iperf_hostpresent ( iperf_sockaddr *find, Iperf_ListEntry *root ) {
    Iperf_ListEntry *itr = root;
    while ( itr != NULL ) {
        if ( SockAddr_Hostare_Equal( (sockaddr*)itr, (sockaddr*)find ) ) {
            return itr;
        }
        itr = itr->next;
    }
    return NULL;
}
