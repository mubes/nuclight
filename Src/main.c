/* SPDX-License-Identifier: BSD-3-Clause */

#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <fcntl.h>
#include <ctype.h>
#include <assert.h>
#include <getopt.h>
#include "asm/ioctls.h"
#include <asm/termios.h>
/* Manual declaration to avoid conflict. */
extern int ioctl ( int __fd, unsigned long int __request, ... ) ;

#include "generics.h"

/* Protocol is based on discoveries made by 'fairedummy' in Reddit r/MiniPCs Dec 2023 in *
 * the thread `T9 Plus N100 - How to control LED'. Kudos to him.
 */

#define DEFAULT_BAUDRATE 10000
#define DEFAULT_SERPORT  "/dev/ttyUSB0"
#define MAX_MODE     5
#define MAX_BRIGHT   5
#define MAX_SPEED    5
#define PACING_MS    5

static const char *_modeName[] = { "","Rainbow","Breathing","Cycle","Off","Auto" };

/* Record for options, either defaults or from command line */
struct Options
{
  char *port;                                          /* Serial host connection */
  int speed;                                           /* Speed of serial link */
};

struct RunTime
{
  int mode;
  int speed;
  int bright;

  struct Options o;
} _r =
{
  .o.port = DEFAULT_SERPORT,
  .o.speed = DEFAULT_BAUDRATE
};

// ====================================================================================================
static void _printHelp( const char *const progName )

{
  genericsPrintf( "Usage: %s [options]" EOL, progName );
  genericsPrintf( "    -a, --serial-speed:  <serialSpeed> to use" EOL );
  genericsPrintf( "    -b, --brightness:    <num> Brightness Level 1..5" EOL );
  genericsPrintf( "    -h, --help:          This help" EOL );
  genericsPrintf( "    -m, --mode:          <mode> ");
  for (int i=1; i<(sizeof(_modeName)/sizeof(_modeName[0])); i++)
    {
      genericsPrintf("%d=%s  ",i,_modeName[i]);
    }
  genericsPrintf( EOL "    -p, --serial-port:   <serialPort> to use" EOL );
  genericsPrintf( "    -s, --speed:         <num> Speed 1..5" EOL );
  genericsPrintf( "    -v, --verbose:       <level> Verbose mode 0(errors)..3(debug)" EOL );
  genericsPrintf( "    -V, --version:       Print version and exit" EOL );
}

// ====================================================================================================
void _printVersion( char *progName )

{
  genericsPrintf( "%s version " GIT_DESCRIBE EOL,progName );
}
// ====================================================================================================
static struct option _longOptions[] =
{
  {"serial-speed", required_argument, NULL, 'a'},
  {"brightness", required_argument, NULL, 'b'},
  {"help", no_argument, NULL, 'h'},
  {"mode", required_argument, NULL, 'm'},
  {"serial-port", required_argument, NULL, 'p'},
  {"speed", required_argument, NULL, 's'},
  {"verbose", required_argument, NULL, 'v'},
  {"version", no_argument, NULL, 'V'},
  {NULL, no_argument, NULL, 0}
};
// ====================================================================================================
bool _processOptions( int argc, char *argv[], struct RunTime *r )

{
  int c, optionIndex = 0;

  while ( ( c = getopt_long ( argc, argv, "a:b:hm:p:s:v:V", _longOptions, &optionIndex ) ) != -1 )
    switch ( c )
      {
        // ------------------------------------
        case 'a':
          r->o.speed = atoi( optarg );
          break;
        // ------------------------------------
        case 'b':
          r->bright = atoi( optarg );
          break;

        // ------------------------------------
        case 'h':
          return false;

        // ------------------------------------

        case 'V':
          _printVersion(argv[0]);
          return false;

        // ------------------------------------

        case 'm':
          r->mode = atoi( optarg );
          if (r->mode == 4)
            {
              /* ...a slight kludge to remove the need for brightness and speed when turning off */
              r->bright = MAX_BRIGHT;
              r->speed = MAX_SPEED;
            }
          break;

        // ------------------------------------

        case 'p':
          r->o.port = optarg;
          break;

        // ------------------------------------

        case 's':
          r->speed = atoi( optarg );
          break;

        // ------------------------------------

        case 'v':
          if ( !isdigit( *optarg ) )
            {
              genericsReport( V_ERROR, "-v requires a numeric argument." EOL );
              return false;
            }

          genericsSetReportLevel( atoi( optarg ) );
          break;

        // ------------------------------------

        case '?':
          if ( optopt == 'b' )
            {
              genericsReport( V_ERROR, "Option '%c' requires an argument." EOL, optopt );
            }
          else if ( !isprint ( optopt ) )
            {
              genericsReport( V_ERROR, "Unknown option character `\\x%x'." EOL, optopt );
            }

          return false;

        // ------------------------------------
        default:
          genericsReport( V_ERROR, "Unrecognised option '%c'" EOL, c );
          return false;
          // ------------------------------------
      }

  /* ... and dump the config if we're being verbose */
  genericsReport( V_INFO, "%s version " GIT_DESCRIBE EOL,argv[0] );
  if ( r->o.port )
    {
      genericsReport( V_INFO, "Serial Port    : %s" EOL, r->o.port );
    }

  if ( r->o.speed )
    {
      genericsReport( V_INFO, "Serial Speed   : %d baud" EOL, r->o.speed );
    }

  if (( !r->mode ) || ( r->mode > MAX_MODE ))
    {
      genericsReport( V_ERROR,"Mode out of range or not set" EOL);
      return false;
    }

  if (( !r->bright ) || ( r->bright > MAX_BRIGHT ))
    {
      genericsReport( V_ERROR,"Brightness out of range or not set" EOL);
      return false;
    }
  if (( !r->speed ) || ( r->speed > MAX_SPEED ))
    {
      genericsReport( V_ERROR,"Speed out of range or not set" EOL);
      return false;
    }

  genericsReport( V_INFO," Config {m,b,s} : %s,%d,%d" EOL, _modeName[r->mode],r->bright,r->speed );

  return true;
}

// ====================================================================================================

static bool _setSerialConfig ( int f, int speed )
{
  // Use Linux specific termios2.
  struct termios2 settings;
  int ret = ioctl( f, TCGETS2, &settings );

  if ( ret < 0 )
    {
      return false;
    }

  settings.c_iflag &= ~( ISTRIP | INLCR | IGNCR | ICRNL | IXON | IXOFF );
  settings.c_lflag &= ~( ICANON | ECHO | ECHOE | ISIG );
  settings.c_cflag &= ~PARENB; /* no parity */
  settings.c_cflag &= ~CSTOPB; /* 1 stop bit */
  settings.c_cflag &= ~CSIZE;
  settings.c_cflag &= ~( CBAUD | CIBAUD );
  settings.c_cflag |= CS8 | CLOCAL; /* 8 bits */
  settings.c_oflag &= ~OPOST; /* raw output */

  settings.c_cflag |= BOTHER;
  settings.c_ispeed = speed;
  settings.c_ospeed = speed;

  ret = ioctl( f, TCSETS2, &settings );

  if ( ret < 0 )
    {
      genericsReport( V_ERROR, "Unsupported baudrate" EOL );
      return false;
    }

  // Check configuration is ok.
  ret = ioctl( f, TCGETS2, &settings );

  if ( ret < 0 )
    {
      return false;
    }

  if ( ( settings.c_ispeed != speed ) || ( settings.c_ospeed != speed ) )
    {
      genericsReport( V_ERROR, "Failed to set baudrate" EOL );
      return false;
    }

  // Flush port.
  ioctl( f, TCFLSH, TCIOFLUSH );
  return true;
}

// ====================================================================================================

static bool _setNucLights( char *port, int serSpeed, int mode, int bright, int speed )

{
  if ((!mode || mode>MAX_MODE) ||
      (!speed || speed>MAX_SPEED) ||
      (!bright || bright>MAX_BRIGHT))
    {
      return false;
    }

  int f,d=0;
  bright=(MAX_BRIGHT+1)-bright;
  speed=(MAX_SPEED+1)-speed;
  unsigned char txPacket[] = { 0xFA,mode,bright,speed,0xfa+mode+bright+speed };
  unsigned char *txp = txPacket;


  f = open( port, O_WRONLY );
  if (f < 0 )
    {
      return false;
    }

  if (!_setSerialConfig ( f, serSpeed ))
    {
      return false;
    }

  while ( d>=0 && txp-txPacket < sizeof(txPacket ) )
    {
      d = write( f, txp, 1 );
      usleep(PACING_MS*1000);
      txp+=d;
    }

  close(f);

  return d>=0;
}

// ====================================================================================================
// ====================================================================================================
// ====================================================================================================
// Publicly available routines
// ====================================================================================================
// ====================================================================================================
// ====================================================================================================

int main( int argc, char *argv[] )

{
  if ( !_processOptions( argc, argv, &_r ) )
    {
      /* processOptions generates its own error messages */
      _printHelp( argv[0] );
      return -1 ;
    }

  if ( !_setNucLights( _r.o.port, _r.o.speed, _r.mode, _r.bright, _r.speed ))
    {
      genericsReport( V_WARN,"Failed to set lights" EOL);
      return -2;
    }

  return 0;
}

// ====================================================================================================
