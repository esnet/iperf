/*
 * iperf, Copyright (c) 2014-2017, The Regents of the University of
 * California, through Lawrence Berkeley National Laboratory (subject
 * to receipt of any required approvals from the U.S. Dept. of
 * Energy).  All rights reserved.
 *
 * If you have questions about your rights to use or distribute this
 * software, please contact Berkeley Lab's Technology Transfer
 * Department at TTD@lbl.gov.
 *
 * NOTICE.  This software is owned by the U.S. Department of Energy.
 * As such, the U.S. Government has been granted for itself and others
 * acting on its behalf a paid-up, nonexclusive, irrevocable,
 * worldwide license in the Software to reproduce, prepare derivative
 * works, and perform publicly and display publicly.  Beginning five
 * (5) years after the date permission to assert copyright is obtained
 * from the U.S. Department of Energy, and subject to any subsequent
 * five (5) year renewals, the U.S. Government is granted for itself
 * and others acting on its behalf a paid-up, nonexclusive,
 * irrevocable, worldwide license in the Software to reproduce,
 * prepare derivative works, distribute copies to the public, perform
 * publicly and display publicly, and to permit others to do so.
 *
 * This code is distributed under a BSD style license, see the LICENSE file
 * for complete information.
 */

#include "iperf_config.h"

#include <time.h>
#include <sys/types.h>
#include <openssl/bio.h>

#ifdef __WIN32__

typedef intptr_t ssize_t;

ssize_t getline(char **lineptr, size_t *n, FILE *stream);

/* Replacement termios.h for MinGW32.
   Copyright (C) 2008 CodeSourcery, Inc.
   This program is free software; you can redistribute it and/or modify
   it under the terms of the GNU General Public License as published by
   the Free Software Foundation; either version 2 of the License, or
   (at your option) any later version.
   This program is distributed in the hope that it will be useful,
   but WITHOUT ANY WARRANTY; without even the implied warranty of
   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
   GNU General Public License for more details.
   You should have received a copy of the GNU General Public License
   along with this program.  If not, see <http://www.gnu.org/licenses/>.  */

typedef unsigned char cc_t;
typedef unsigned int tcflag_t;

#define NCCS 18

struct termios
{
  tcflag_t c_iflag;
  tcflag_t c_oflag;
  tcflag_t c_cflag;
  tcflag_t c_lflag;
  cc_t c_cc[NCCS];
};

#define VEOF 0
#define VEOL 1
#define VERASE 2
#define VINTR 3
#define VKILL 4
#define VMIN 5
#define VQUIT 6
#define VSTART 7
#define VSTOP 8
#define VSUSP 9
#define VTIME 10
#define VEOL2 11
#define VWERASE 12
#define VREPRINT 13
#define VLNEXT 15
#define VDISCARD 16

#define _POSIX_VDISABLE '\0'

#define BRKINT 0x1
#define ICRNL 0x2
#define IGNBRK 0x4
#define IGNCR 0x8
#define IGNPAR 0x10
#define INLCR 0x20
#define INPCK 0x40
#define ISTRIP 0x80
#define IXANY 0x100
#define IXOFF 0x200
#define IXON 0x400
#define PARMRK 0x800

#define OPOST 0x1
#define ONLCR 0x2
#define OCRNL 0x4
#define ONOCR 0x8
#define ONLRET 0x10
#define OFILL 0x20

#define CSIZE 0x3
#define CS5 0x0
#define CS6 0x1
#define CS7 0x2
#define CS8 0x3
#define CSTOPB 0x4
#define CREAD 0x8
#define PARENB 0x10
#define PARODD 0x20
#define HUPCL 0x40
#define CLOCAL 0x80

#define ECHO 0x1
#define ECHOE 0x2
#define ECHOK 0x4
#define ECHONL 0x8
#define ICANON 0x10
#define IEXTEN 0x20
#define ISIG 0x40
#define NOFLSH 0x80
#define TOSTOP 0x100
#define FLUSHO 0x200

#define TCSANOW 0
#define TCSADRAIN 1
#define TCSAFLUSH 2

#define TCIOFF 0
#define TCION 1
#define TCOOFF 2
#define TCOON 3

int tcgetattr (int fd, struct termios *buf);
int tcsetattr (int fd, int actions, const struct termios *buf);
int tcdrain (int fd);
int tcflow (int fd, int action);

/* We want to intercept TIOCGWINSZ, but not FIONREAD.  No need to forward
   TIOCSWINSZ; readline only uses it to suspend if in the background.
   Readline doesn't make any other ioctl calls on mingw.  */

#include <winsock.h>

struct winsize
{
  unsigned short ws_row;
  unsigned short ws_col;
};

int mingw_getwinsize (struct winsize *window_size);
#define TIOCGWINSZ 0x42424240
#define TIOCSWINSZ 0x42424241
#define ioctl(fd, op, arg)				\
  (((op) == TIOCGWINSZ) ? mingw_getwinsize (arg)	\
   : ((op) == TIOCSWINSZ) ? -1				\
   : ioctlsocket (fd, op, arg))

#endif /* win32 */

int test_load_pubkey_from_file(const char *public_keyfile);
int test_load_private_key_from_file(const char *private_keyfile);
EVP_PKEY *load_pubkey_from_file(const char *file);
EVP_PKEY *load_pubkey_from_base64(const char *buffer);
EVP_PKEY *load_privkey_from_file(const char *file);
EVP_PKEY *load_privkey_from_base64(const char *buffer);
int encode_auth_setting(const char *username, const char *password, EVP_PKEY *public_key, char **authtoken);
int decode_auth_setting(int enable_debug, const char *authtoken, EVP_PKEY *private_key, char **username, char **password, time_t *ts);
int check_authentication(const char *username, const char *password, const time_t ts, const char *filename);
ssize_t iperf_getpass (char **lineptr, size_t *n, FILE *stream);
