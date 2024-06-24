
/*+---------------------------------------------------------------------------+*/
/*                                                                             */
/*                        fractz                                               */
/*                  a fractal generater                                        */
/*               Copyright (c)  Tom Stokes  1993, 1994                                    */
/*+---------------------------------------------------------------------------+*/


/*+--------------------------------------------------------------------------+*/
/*| System and library header files.                                         |*/
/*+--------------------------------------------------------------------------+*/

#define _FP_INLINE              /* generate inline FPU code */
#define INCL_WIN
#define INCL_DEV
#define INCL_GPI
#define INCL_DOS

#include <os2.h>
#include <stdlib.h>
#include <stdio.h>
#include <string.h>
//#include <math.h>
//#include <bsememf.h>
#include <complex.h>

/*+--------------------------------------------------------------------------+*/
/*| Application header files.                                                |*/
/*+--------------------------------------------------------------------------+*/

#include "fractz.h"

/*+--------------------------------------------------------------------------+*/
/*| Internal function prototypes.                                            |*/
/*+--------------------------------------------------------------------------+*/

static void DrawingThread(void);  // thread
static void Sleeper(void);        // thread
static void Colorer(void);        // thread

static void InitTitle( HAB, HWND, char *, char *);
static void UpdateTitle(HWND, char *, char *);

static MRESULT EXPENTRY ClientWindowProc( HWND hwnd, ULONG msg, MPARAM mp1, MPARAM mp2 );
static MRESULT EXPENTRY SettingsDlg(HWND, ULONG, MPARAM, MPARAM);
static MRESULT EXPENTRY HelpDlgProc( HWND hwndDlg, ULONG msg, MPARAM mp1, MPARAM mp2 );

static void DisplayMessage( HAB, ULONG );

/*+--------------------------------------------------------------------------+*/
/*| Static global variables and local constants.                             |*/
/*+--------------------------------------------------------------------------+*/

#define WINDOW_CLASS   "My_Window"          /* window class name              */
#define MSG_BOX_ID     256                  /* error message box id           */
#define STRING_LENGTH  128                  /* size of buffer used for strings*/
#define STACK_SIZE     8192UL               /* stack size for second thread   */
#define PERIOD_CHECK                        /* check for periodicity          */

//////////////////////////////////////////////////////////////////////////////////
//   note on C/set++ &  __BCPLUSPLUS__
// C/set++ defines PCSZ as: typedef const char *PCSZ and then uses it
// as the parameter type in many APIs. BCOS2, on the other hand, does not
// define PCSZ but instead uses: typedef unsigned char *PSZ.
// Thus the following typedef within the __BCPLUSPLUS__ pre-defined macro
// allows living in both worlds.
//////////////////////////////////////////////////////////////////////////////////

#ifdef __BCPLUSPLUS__
#define min(x,y)  (((x<y) ? x:y))
#define max(x,y)  (((x<y) ? y:x))
typedef unsigned char *PCSZ;
#endif

/*+---------------------------------------------------------------------------+*/
/*|   defines for some of the classic fractal types                           |*/
/*+---------------------------------------------------------------------------+*/

#define MANDEL        1
#define MANDELZ       "Mandelbrot"
#define LAMBDA        2
#define LAMBDAZ       "Lambda"
#define CMPLXNEWT      3
#define CMPLXNEWTZ     "Cmplx Newton"
#define JULIA1        4
#define JULIAZ        "Julia"
#define COMPLEXMAND   5
#define COMPLEXMANDZ  "Cmplx Mark's Mandelbrot"
#define JULIACLICK    6   // swith to via mouse click (NOT IN MENU)
//#define JULIAZ       "Julia"   already defined
#define MAGNETJ1      7
#define MAGNETJ1Z     "Magnetism Julia I"
#define TETRATION     8
#define TETRATIONZ    "Tetration"
#define SPIDER        9
#define SPIDERZ       "Spider"

static HWND   hwndFrame = NULLHANDLE;       // frame window handle
static HWND   hwndClient = NULLHANDLE;      // client window handle
static HWND   hwndMenu = NULLHANDLE;        // menu  window handle
static HMQ    hmqThread2 = NULLHANDLE;      // message queue handle for thread
static HPS    hpsMem = NULLHANDLE;          // for bitmap
static HBITMAP hbm;
static PBITMAPFILEHEADER2 pbmpfileheader2;  // now for read bitmap from file
static PBITMAPINFO2 pbmp2;                  // file
static HPS    hpsfile = NULLHANDLE;         // file
static PBYTE  pBitsfile;                    // bitmap data from file
static HBITMAP hbmfile;                     // bitmap handle
static SHORT  cxClient, cyClient;           // client window X and Y size
static SHORT  cxCold, cyCold;
static SHORT  cxfilebmp, cyfilebmp;         // size of bmp from file
static TID    tidDrawing;                   // drawing thread identifier
static TID    tidSleeper;                   // sleeper thread id.             */
static TID    tidColorer;
static char   szAppName[ MAXNAMEL + 1 ];    // application name string
static CHAR fractnamestr[40];
static CHAR FullTitleText[100];            // concat of appname + fractname
static  LONG rdown[256],                    // boolean for color intensity going down
             gdown[256],
             bdown[256];

static HPS    hpsorb;
static HAB    hab = NULLHANDLE;               // PM anchor block handle
static HDC    hdcMem;
static PBYTE  pBitmap = NULL;                 // creater stores into this
static HMTX   sema4;                          // mutex sem
static HMTX   semsleeper;                     // mutex sem
static HMTX   semcolorer;                     // mutex sem
static PID    pidProcess;                     // process ident
static APIRET semrslt;                        // result of sem op (create,.etc.)
static PCSZ semname  = "\\SEM32\\STARTUP";    // name of semaphore
static PCSZ sem2name = "\\SEM32\\SLEEPER";    // name of semaphore 2
static PCSZ sem3name = "\\SEM32\\COLORER";    // name of semaphore 3
static PBITMAPINFO2 pbmi;

static int
         showorbits = 0,
         colorit = 1,
         changecolors = 0,
         ix,
         iy,
         color,
         insidecolor = 1,
         usepotential = 0,
         iter,
         maxiter = 255,
         maxcolor1,
         maxxp,
         maxyp,
         periodicity,
         lastperiod,
         checkevery,
         finiteattractor,   // we use this 'cause has own convergence criteria
         fractype,
         three_d = 0;

static ULONG                    // globals for remembering what menu items have checks
         CurrentFractalChecked,
         CurrentMapChecked,
         CurrentImageOrbitsChecked,
         CurrentFixChangeColorsChecked,
         databytes;            // size of bitmap


static   BOOL
         CheckComplementAgain = FALSE,   // on/off toggle
         spherical = FALSE,
         sphere_covers_180 = FALSE,      // 180 or 360
         autoscale = TRUE;


static double
         x,
         y,
         savedx,
         savedy,
         xperturb = 0.0,    // perturbations - can be applied to z, c, p...
         yperturb = 0.0,
         xl,               // xleft, xright,ybottom, ytop in fractal units
         xr,
         yb,
         yt,
         dx,
         dy,
         xlv,
         xrv,
         ytv,
         ybv,
         dxv,              // xr - xl
         dyv,              // yt - yb
         rmaxxp,
         rmaxyp,
         pixelepsilon,  // convergence criteria - name means the value is pixel/zoom based
         potential,
         bailout = 2.0,
         maxheight,
         alpha,
         beta,
         phi,
         sa,        // sin(alpha)
         ca,        // cos(alpha)
         sb,        // sin(beta)
         cb,        // cos(beta)
         sp,        // sin(phi)
         cp,        // cos(phi)
	 pi,
         rpd,
         sphererad = 1.0,
         theta,
         rho,
         xlp, // these are the 6 min,max pts. of the zoom box in the 3-d system
         xrp, // x-right-primed....
         ybp,
         ytp,
         zbp,
         ztp,
         dxvp,  // xr-xl etc. but in primed (3-d rotated) coord. system
         dyvp,
         dzvp,
         xlps,  // 3-d rotated sphere
         xrps,
         ybps,
         ytps,
         zbps,
         ztps,
         dxvps,
         dyvps,
         dzvps;

static complex
         z,
         c,
         c0,       // constant c for Julias
         savedz,   // previos z for periodicity checking
         t1, t2,   // temporaries to simplify expressions
         zperturb; // perturbation z

static QMSG  qmsgThread2;   // message queue

static LONG blTable[256],   // color tables
            alTable[256];

/*+--------------------------------------------------------------------------+*/
/*| Dummy argument parsing routine so that real one does not get linked in.  |*/
/*+--------------------------------------------------------------------------+*/

 void _setuparg( void ) {}

/*+--------------------------------------------------------------------------+*/
/*| Main control procedure.                                                  |*/
/*+--------------------------------------------------------------------------+*/

int main( void )
{
   HMQ   hmq = NULLHANDLE;
   ULONG flCreate = 0UL;
   QMSG  qmsg;
   int   rc = 1;

   do
   {
      // sem requests must be early on.

      semrslt = DosCreateMutexSem(semname, &sema4, 0, FALSE);
      semrslt = DosRequestMutexSem(sema4, SEM_INDEFINITE_WAIT);

      semrslt = DosCreateMutexSem(sem2name, &semsleeper, 0, FALSE);
      semrslt = DosRequestMutexSem(semsleeper, SEM_INDEFINITE_WAIT);

      semrslt = DosCreateMutexSem(sem3name, &semcolorer, 0, FALSE);
      semrslt = DosRequestMutexSem(semcolorer, SEM_INDEFINITE_WAIT);

      // Initialize PM and create a message queue of default size.

      if ( ( hab = WinInitialize( 0UL ) ) == NULLHANDLE )
         break;

      if ( ( hmq = WinCreateMsgQueue( hab, 0UL ) ) == NULLHANDLE )
         break;

      // Register client window class

      if ( !WinRegisterClass( hab,          // PM anchor block handle
                              WINDOW_CLASS, // window class name
                          ClientWindowProc, // address of window procedure
                              CS_SIZEREDRAW,// size changes cause redrawing
                              0UL ) )       // window data
      {
         DisplayMessage( hab, IDS_NOREGISTER );
         break;
      }

      flCreate = FCF_STANDARD & ~FCF_TASKLIST;

      hwndFrame =
         WinCreateStdWindow( HWND_DESKTOP,  // make desktop window the parent
                             WS_VISIBLE,    // frame window class style
                             &flCreate,     // frame control flags
                             WINDOW_CLASS,  // client window class name
                             "",            // title bar text
                             CS_SIZEREDRAW, // client window class style
                             0UL,           // resource file handle - in EXE
                             ID_WINDOW,     // resources identifier
                             &hwndClient ); // client window handle

      if ( hwndFrame == NULLHANDLE )
      {
         DisplayMessage( hab, IDS_NOSTDWINDOWS );
         break;
      }

      hpsorb = WinGetPS(hwndClient);    // ps for drawing orbits

      // Initialize the window title and task switch list

      InitTitle( hab, hwndFrame, szAppName, MANDELZ );

      // Create the thread that will draw the lines.

      if ( DosCreateThread( &tidDrawing, (PFNTHREAD)DrawingThread, 0UL, 0UL, STACK_SIZE ) )
      {
         DisplayMessage( hab, IDS_NOTHREAD );
         break;
      }

      if ( DosCreateThread( &tidSleeper, (PFNTHREAD)Sleeper, 0UL, 0UL, STACK_SIZE ) )
      {
         DisplayMessage( hab, IDS_NOTHREAD );
         break;
      }

      if ( DosCreateThread( &tidColorer, (PFNTHREAD)Colorer, 0UL, 0UL, STACK_SIZE ) )
      {
         DisplayMessage( hab, IDS_NOTHREAD );
         break;
      }

      // While the WM_QUIT message is not received, dispatch the message.

      while( WinGetMsg( hab, &qmsg, 0UL, 0UL, 0UL ) )
         WinDispatchMsg( hab, &qmsg );
      rc = 0;
      break;

   }  while ( FALSE );

   if ( hwndFrame != NULLHANDLE )
      WinDestroyWindow( hwndFrame );
   if ( hmq != NULLHANDLE )
      WinDestroyMsgQueue( hmq );
   if ( hab != NULLHANDLE )
      WinTerminate( hab );

   return rc;
}

/*+--------------------------------------------------------------------------+*/
/*        start of thread2 code                                               */
/*+--------------------------------------------------------------------------+*/
ixp(double x)   /* return x-pixel value given x */
{
        return(maxxp * (x-xlv)/dxv);
}

iyp(double y)   /* return y-pixel value given y  */
{
        return(maxyp * (y-ybv)/dyv);
}

// as above, but in primed (rotated) coord system
ixpp(double x)   /* return x-pixel value given x */
{
        return(maxxp * (x-xlp)/dxvp);
}

iypp(double y)   //
{
        return(maxyp * (y-ybp)/dyvp);
}

izpp(double z)
{
        return(maxyp * (z-zbp)/dzvp);
}

// as above, but now a sphere in primed (rotated) coord system
ixpps(double x)   /* return x-pixel value given x */
{
        return(maxxp * (x-xlps)/dxvps);
}

iypps(double y)   //
{
        return(maxyp * (y-ybps)/dyvps);
}

izpps(double z)
{
        return(maxyp * (z-zbps)/dzvps);
}

void rotpoint(double x1, double y1, double z1, double *x2, double *y2, double *z2 )
{
    *x2 =  x1*cb*cp - y1*cb*sp + z1*sb;
    *y2 =  x1*(sa*sb*cp+sp*ca) + y1*(ca*cp-sa*sb*sp) - z1*sa*cb;
    *z2 =  x1*(sa*sp-ca*sb*cp) + y1*(sa*cp+ca*sb*sp) + z1*ca*cb;
    return;
}

void sphericalpoint(double x1, double y1, double z1, double *x2, double *y2, double *z2)
{
   double rrad, trad;
   double sr,cr,st,ct;
   if (sphere_covers_180)
      theta = (x1-xl)/dx*180.0 - 90.0;       // 180
   else
      theta = (x1-xl)/dx*360.0 - 180.0;      // 360
   rho =   (y1-yb)/dy*180.0 - 90.0;          // +- 90
   trad = theta * rpd;
   rrad = rho * rpd;
   cr = cos(rrad);
   sr = sin(rrad);
   st = sin(trad);
   ct = cos(trad);
   *x2 = (sphererad+z1)*cr*st;
   *y2 = (-sphererad+z1)*cr*ct;
   *z2 = (sphererad+z1)*sr;
}

void initializechecking(void)  /* used in periodicity checking for early bailout */
{
   savedz = complex(0,0);
   periodicity = lastperiod = 1;
   checkevery = 3;
}

periodicitycheck(void)
{
   complex dz;
   dz = z - savedz;
   if (abs(dz)< pixelepsilon) {
     if (lastperiod==periodicity)
        return periodicity;
     else {
        lastperiod = periodicity;
        periodicity = 1;
     }
   }
   else {
     if (periodicity++ > checkevery) {
        checkevery *= 2;
        periodicity = 1;
        savedz = z;
     }
   }
   return 0;
}


fractalpoint(double cx, double cy, int fractype)
{
  POINTL ptl;
  complex n,r,p,zprev,deltaz;
  double arg, x2;
  int ret;
  iter = 0;
  potential = maxheight;
#ifdef PERIOD_CHECK
  initializechecking();  /* look for periodicity */
#endif
  switch (fractype) {
    case MANDEL: // mandelbrot
      c = complex(cx, cy);
      z = complex(xperturb,yperturb);
      break;
    case LAMBDA:  // lambda
      c = complex(xperturb, yperturb);
      z = complex(cx, cy);
      break;
    case JULIA1:  // julia example - no choice on 'c'
      c = complex(xperturb, yperturb);
      z = complex(cx, cy);
      break;
    case CMPLXNEWT: // complex newton
      n = complex(10,3);
      r = complex(xperturb, yperturb);
      z = complex(cx,cy);
      break;
    case COMPLEXMAND: // complex mandelbrot
      c = complex(cx,cy);
      p = complex(xperturb, yperturb);
      z = complex(0,0);
      break;
    case JULIACLICK: // switch to Julia via mouse click
      z = complex(cx, cy);   // note - we already have c0 from mouse click pos
      break;
    case MAGNETJ1:
      c = complex(xperturb, yperturb);
      z = complex(cx, cy);
      break;
    case TETRATION:
      z = complex(cx, cy);
      c = z;
    break;
    case SPIDER :
      z = complex(cx,cy);
      c = z;
    break;
  default :
      break;
  } // end switch
  while (iter < maxiter) { // iteration loop starts here
    switch (fractype) {
      case MANDEL:
        z = z*z + c;
        break;
      case LAMBDA:
        z = c*z*(1.0-z);
        break;
      case JULIA1:
        z = z*z + c;
        break;
      case CMPLXNEWT:
        zprev = z;
        z = ((n-1.0)*pow(z,n)+r)/(n*pow(z,n-1));
        deltaz = z - zprev;
        break;
      case COMPLEXMAND:
        z = z*z*pow(c,p-1) + c;
        break;
      case JULIACLICK:  // switch to Julia
        z = z*z + c0;
        break;
      case MAGNETJ1:
        t1 = z*z+c-1.0;
        t1 = t1*t1;
        t2 = 2.0*z+c-2.0;
        t2 = t2*t2;
        z = t1/t2;
        break;
      case TETRATION:
        z = pow(c,z);
        break;
      case SPIDER :
        z = z*z + c;
        c = c/2.0 + z;
        break;
      default:
        break;
    } // end switch
    if ((showorbits) && (iter>0)) {
       x = real(z);
       y = imag(z);
       ptl.x = (x-xlv)/dxv*maxxp;
       ptl.y = (y-ytv)/dyv*rmaxyp+rmaxyp;
       GpiMove(hpsorb,&ptl);
       GpiSetColor(hpsorb, iter%maxcolor1);
       GpiSetPel(hpsorb, &ptl);
    }
    iter++;
#ifdef PERIOD_CHECK             // can't check periodicity with finit attracters
    if ((iter % checkevery == 0) && (!finiteattractor) && (!usepotential)) {
        if (periodicitycheck())
           return insidecolor;
     }
#endif
    if (finiteattractor) {
        if ( abs(deltaz) <  pixelepsilon)
           return iter % maxcolor1;
     }
     else {
       if (abs(z) > bailout) {
          if (usepotential) {
             //potential = maxcolor1*(1.0-log(abs(z))/pow(2.0, iter));
             arg = min(abs(z)/bailout, 1.0);
             potential = maxcolor1*(1.0-log(arg));
             ret = (int)ceil(potential) % maxcolor1;
          }
          else
            ret = iter % maxcolor1;
         return ret;
       }
    }
  } // end while
  return insidecolor;
}


void fractalx(void)
{
   /* - this operates on x. (does one y-line then checks msgs) -*/
      double
         cx,
         cy,
         cxparam,
         cyparam,
         czparam,
         xprime,
         yprime,
         zprime;  // 3-D rotated pts.  Do Not confuse with type complex
      POINTL
         ptl;
      int
         xpixel,
         ypixel,
         pinx;
      PBYTE  pBitmaptemp,         // aviod excessive indexing
             pBitmapditherup,     // needs dithering both directions, highly non-linear
             pBitmapditherahead;
      maxxp = cxClient-1;
      maxyp = cyClient-1;
      rmaxxp = (double) maxxp;
      rmaxyp = (double) maxyp;
      xl = xlv;
      xr = xrv;
      yb = ybv;
      yt = ytv;
      dx = xr - xl;
      dxv = xrv - xlv;
      dyv = ytv - ybv;
      if (!spherical)
         dy = yt - yb;
      else
      {
          // use this for square squares, round spheres, etc.
          dy = dx * rmaxyp / rmaxxp;
          yt = dy/2.0;
          yb = -yt;
      }
      pixelepsilon = dx/(2.0*rmaxxp);    // ref. Bert Tyler - the 2.0 is from 1/2 pixel
      cy = ytv + (double)(iy - maxyp) * dyv/rmaxyp;
      ix = 0;
      finiteattractor = (fractype == CMPLXNEWT);  // or in some more if needed
      while (ix <= maxxp) {
          cx = dxv * (double) ix/rmaxxp + xlv;
          color = fractalpoint(cx, cy, fractype);
          // cxparam, etc, are cx, etc. for switched co-ord systems.
          cxparam = dx * (double) ix/rmaxxp + xl;
          cyparam = yt + (double)(iy-maxyp) * dy/rmaxyp;

          if ((three_d) && (!spherical)) {
               if (usepotential)
                  czparam = potential/maxheight;
               else
                  czparam = (double) color/maxheight;
               rotpoint(cxparam, cyparam, czparam, &xprime, &yprime, &zprime);
               if (autoscale) {
                  xpixel = ixpp(xprime);
                  ypixel = izpp(zprime);
               }
               else {
                  xpixel = ixp(xprime);
                  ypixel = iyp(zprime);
               }
               pinx = xpixel + cxClient*ypixel;
               pBitmaptemp = &pBitmap[pinx];         // saves on index op (below)
               if ((xpixel > 0) && (xpixel < maxxp)   // prevent an access violation
                    && (ypixel > 0) && (ypixel < maxyp)
                    && (*pBitmaptemp == 0) )  // this line for ray tracing
                          *pBitmaptemp = (UCHAR) color;
          }
          if (spherical) {
               if (usepotential)
                  czparam = potential/maxheight*sphererad/10.0;    // cut by 1/10 if spherical
               else
                  czparam = (double) color/maxheight*sphererad/10.0;
              sphericalpoint(cxparam, cyparam, czparam, &xprime, &yprime, &zprime);
              if (three_d)
                  rotpoint(xprime, yprime, zprime, &xprime, &yprime, &zprime);
              if (yprime <= 0.0) { // show near side only
                  if (autoscale) {
                      xpixel = ixpps(xprime);
                      ypixel = izpps(zprime);
                  }
                  else {
                      xpixel = ixp(xprime);
                      ypixel = iyp(zprime);
                  }
                  // since another thread could be allocating/deallocating the
                  // bitmap array, it must be protected. A mutex sem would probably
                  // be too expensive just to protect an index-store type op.
                  DosEnterCritSec();
                  pinx = xpixel + cxClient*ypixel;
                  pBitmaptemp = &pBitmap[pinx];
                  pBitmapditherahead = pBitmaptemp;
                  pBitmapditherup = pBitmaptemp;
                  if ((xpixel > 0) && (xpixel < maxxp)   // prevent an access violation
                       && (ypixel > 0) && (ypixel < maxyp) )
                           *pBitmaptemp = (UCHAR) color;

                  // now dither twice - both up and ahead
                  if ((xpixel > 0) && (xpixel < maxxp)
                       && (ypixel+2 > 0) && (ypixel+2 < maxyp))  // up
                  {
                            pBitmapditherup += cxClient;
                           *pBitmapditherup = (UCHAR) color;
                            pBitmapditherup += cxClient;
                           *pBitmapditherup = (UCHAR) color;
                  }
                  if ((xpixel+2 > 0) && (xpixel+2 < maxxp)      // ahead
                       && (ypixel > 0) && (ypixel < maxyp))
                  {
                            pBitmapditherahead++;
                           *pBitmapditherahead = (UCHAR) color;
                            pBitmapditherahead++;
                           *pBitmapditherahead = (UCHAR) color;
                  }
                  DosExitCritSec();
              } // end if y<=0
           }
           if ((!spherical) && (!three_d)) {
             DosEnterCritSec();
             // several things could happen here. cxClient, cyClient could change,
             // and the bitmap array could be de-allocated.
             if ((ix < cxClient) && (iy < cyClient))
               pBitmap[ix+cxClient*iy] = color;
             DosExitCritSec();
           }
          ix++;
      } // end while(ix..)
      iy++;
      if (iy > maxyp)
        iy = 0;
      return;
}

/*+--------------------------------------------------------------------------+*/
/*| DrawingThread - Procedure that draws in the client window.               |*/
/*+--------------------------------------------------------------------------+*/

static void DrawingThread( void )
{
   HAB   habThread2 = NULLHANDLE;           /* anchor block handle for thread */
   HPS   hps = NULLHANDLE;                  /* presentation Space handle      */
   RECTL rclClient;                         /* rectangle for client window    */

   int ci, i;

   // Change the priority class of this thread
   // PRTYC_REGULAR, PRTYC_IDLETIME, PRTYC_TIMECRITICAL, PRTYC_NOCHANGE
   DosSetPriority( PRTYS_THREAD, PRTYC_IDLETIME, 0L, 0UL );

   habThread2 = WinInitialize( 0UL );
   hmqThread2 = WinCreateMsgQueue( habThread2, 0UL );

   hps = WinGetPS( hwndClient );

   GpiSavePS(hps);

   semrslt = DosRequestMutexSem(sema4, SEM_INDEFINITE_WAIT);
   semrslt = DosReleaseMutexSem(sema4);

   qmsgThread2.msg = WM_PAINT;  // value = anything != WM_USER_END_THREAD

   /* Run fractal line (y=const) in thread2, then check msgs  */

   while ( qmsgThread2.msg != WM_USER_END_THREAD )  {
        fractalx();
        WinPeekMsg( habThread2, &qmsgThread2, NULLHANDLE, 0UL, 0UL, PM_REMOVE );
   }

   /* Clean up and terminate drawing thread. */

   GpiRestorePS(hps, -1L);
   WinReleasePS( hps );
   WinReleasePS(hpsorb);
   WinDestroyMsgQueue( hmqThread2 );
   WinTerminate( habThread2 );
   DosExit( EXIT_THREAD, 0UL );
}

/*+--------------------------------------------------------------------------+*/
/*            End of thread2                                                 +*/
/*+--------------------------------------------------------------------------+*/

/*---------------------------------------------------------------------------+*/
/*            Start of threads 3 & 4                                         */
/*+--------------------------------------------------------------------------+*/

static void Sleeper(void) /* #3 */
{
   semrslt = DosRequestMutexSem(semsleeper, SEM_INDEFINITE_WAIT);
   semrslt = DosReleaseMutexSem(semsleeper);
   while(1) {
       DosSleep(3000);
       WinInvalidateRect(hwndClient, NULL, FALSE);
   }
}

static void Colorer(void) /* #4 */
{
   int i;
   LONG r,g,b, rswitch, gswitch, bswitch;

// these are delta colors that get added or subtracted to the rgb table entries.
// They are small prime numbers to defer periodicity. You could think of them
// as causing traveling waves up and down the table and traveling at different speeds.
#define   dr  3
#define   dg  5
#define   db  7

   semrslt = DosRequestMutexSem(semcolorer, SEM_INDEFINITE_WAIT);
   semrslt = DosReleaseMutexSem(semcolorer);
   while(1) {
       DosSleep(2523);  // weird number just so won't get harmonic with Sleeper thread
       if (changecolors) {
             for (i=1; i<maxcolor1; i++) {
                   r = (blTable[i] & 0x00ff0000) >> 16;
                   g = (blTable[i] & 0x0000ff00) >> 8;
                   b = (blTable[i] & 0x000000ff);

                   rswitch = ((r >= 255-dr) & !rdown[i]) | ((r <= dr) & rdown[i]);
                   gswitch = ((g >= 255-dg) & !gdown[i]) | ((g <= dg) & gdown[i]);
                   bswitch = ((b >= 255-db) & !bdown[i]) | ((b <= db) & bdown[i]);

                   if (rswitch) rdown[i] = !rdown[i];
                   if (gswitch) gdown[i] = !gdown[i];
                   if (bswitch) bdown[i] = !bdown[i];

                   if (rdown[i]) r-=dr-1; else r+=dr;
                   if (gdown[i]) g-=dg-1; else g+=dg;
                   if (bdown[i]) b-=db-1; else b+=db;

                   blTable[i] = (r<<16) + (g<<8) + b;

             }
       }// end if
   } // end while
}


/*+--------------------------------------------------------------------------+*/
/*            End of threads 3 & 4                                           +*/
/*+--------------------------------------------------------------------------+*/

/*---------------------------------------------------------------------------+*/
/*          Back to thread1                                                   */
/*+--------------------------------------------------------------------------+*/

/*+--------------------------------------------------------------------------+*/
/*| InitTitle - Initializes window title and task switch list.               |*/
/*+--------------------------------------------------------------------------+*/


static void InitTitle( HAB hab, HWND hwnd, char *szTitle, char *fractname )
{
   SWCNTRL tSwcntrl;                        /* task switch structure          */

   /* Load the application name from the resources in the EXE.  Set the title */
   /* bar text to this name.  Then add this application to the task manager   */
   /* list with the loaded application name.                                  */

   WinLoadString( hab, 0UL, IDS_APPNAME, MAXNAMEL, szTitle );
   UpdateTitle(hwnd, szTitle, fractname);

   tSwcntrl.hwnd = hwnd;
   tSwcntrl.hwndIcon = NULLHANDLE;
   tSwcntrl.hprog = NULLHANDLE;
   tSwcntrl.idProcess = 0;
   tSwcntrl.idSession = 0;
   tSwcntrl.uchVisibility = SWL_VISIBLE;
   tSwcntrl.fbJump = SWL_JUMPABLE;
   strcpy( tSwcntrl.szSwtitle, szTitle );
   tSwcntrl.bProgType = PROG_PM;
   WinAddSwitchEntry( &tSwcntrl );

   return;
}

static void UpdateTitle(HWND hwnd, char * szAppName, char * fractname)
{
    strcpy(FullTitleText, szAppName);
    strcat(FullTitleText, " - ");
    strcat(FullTitleText, fractname);
    WinSetWindowText(hwnd, FullTitleText);
}

BOOL GetFileName(ULONG WichDialog, char szFullPath[])
{
    FILEDLG
         fdlg;

    fdlg.cbSize = sizeof(FILEDLG);       // Size of FILEDLG structure.
                                         // FDS_ flags. Alter behavior of dlg.
                                         //   FDS_SAVEAS_DIALOG or FDS_OPEN_DIALOG
                                         //   are args to this func
    fdlg.fl =  WichDialog | FDS_ENABLEFILELB;
    fdlg.ulUser = 0;                     // User defined field.
    fdlg.lReturn = 0;                    // Result code from dialog dismissal.
    fdlg.lSRC = 0;                       // System return code.
    fdlg.pszTitle = NULL;                // String to display in title bar.
    fdlg.pszOKButton = NULL;             // String to display in OK button.
                                         // Entry point to custom dialog proc.
    fdlg.pfnDlgProc = (PFNWP)NULL;
    fdlg.pszIType = NULL;                // Pointer to string containing
                                         //   initial EA type filter. Type
                                         //   does not have to exist in list.
    fdlg.papszITypeList = NULL;          // Pointer to table of pointers that
                                         //   point to null terminated Type
                                         //   strings. End of table is marked
                                         //   by a NULL pointer.
    fdlg.pszIDrive = NULL;               // Pointer to string containing                                         //   initial drive. Drive does not
                                         //   have to exist in drive list.
    fdlg.papszIDriveList = NULL;         // Pointer to table of pointers that
                                         //   point to null terminated Drive
                                         //   strings. End of table is marked
                                         // by a NULL pointer.
    fdlg.hMod = (HMODULE)0;              // Custom File Dialog template.
    strcpy(fdlg.szFullFile, "*.BMP"); // Initial or selected fully
                                         //   qualified path and file.
    fdlg.papszFQFilename = NULL;         // Pointer to table of pointers that
                                         //   point to null terminated FQFname
                                         //   strings. End of table is marked
                                         //   by a NULL pointer.
    fdlg.ulFQFCount = 0;                 // Number of files selected
    fdlg.usDlgId = 0;                    // Custom dialog id.
    fdlg.x = 0;                          // X coordinate of the dialog
    fdlg.y = 0;                          // Y coordinate of the dialog
    fdlg.sEAType = 0;                    // Selected file's EA Type.

    // call up file dialog
    if (!WinFileDlg(HWND_DESKTOP, hwndClient, (PFILEDLG)&fdlg))
        return TRUE;

    if (fdlg.lReturn != DID_OK)
        return TRUE;

    strcpy(szFullPath, fdlg.szFullFile);

    return FALSE;  // aok

} // End of GetFileName()

VOID DrawBoxOutline(HWND hwnd, POINTL *pptlStart, POINTL *pptlEnd)
{
     HPS hps;

     hps = WinGetPS(hwnd);
     GpiSetMix(hps, FM_INVERT);
     GpiMove(hps, pptlStart);
     GpiBox(hps, DRO_OUTLINE, pptlEnd, 0L, 0L);
     WinReleasePS(hps);
}


LoadColorTable(char *fn, LONG *cTable, int maxcolor1)
{
  FILE *fin;
  int
        index,
        count,
        red,
        green,
        blue;
#ifdef __BCPLUSPLUS__
const  char  *openmode = "rt";
#else
const  char  *openmode = "r";
#endif

  #define MAXLINELENGTH 90
  char line[MAXLINELENGTH];
  if ( (fin=fopen(fn, openmode)) == NULL)
     return 1;

  index = 0;
  while (index < maxcolor1) {
      if (fgets(line, MAXLINELENGTH,fin) == NULL) {
          fclose(fin);
          return 1;
      }
      count = sscanf(line,"%d %d %d %d",&red, &green, &blue);
      if (count < 3) {
         fclose(fin);
         return 1;
      }
       cTable[index] = ((red & 0xff) << 16) +
                       ((green & 0xff) << 8) +
                        (blue & 0xff);
       index++;
  }
  fclose(fin);
  if (index < maxcolor1)
     return 1;
  return 0;
}

VOID LoadColors(char *fn, LONG* blTable, int maxcolor1)
{
    int i;

    if (LoadColorTable(fn, blTable, maxcolor1)) { //  if fails, make it random
         blTable[0] = 0;
         for (i=1; i<maxcolor1; i++)
         {
          blTable[i] = ((rand() & 0xff) << 16) +
                       ((rand() & 0xff) << 8) +
                        (rand() & 0xff);

          }
    }
}

VOID CheckMenu(ULONG id, BOOL fCheck)
{
    WinSendMsg(hwndMenu,                //global main menu handle
       MM_SETITEMATTR,                  // set menu attribute
       MPFROM2SHORT(id, TRUE),          //menu id
       MPFROM2SHORT(MIA_CHECKED,        // mask = check bit
       fCheck ? MIA_CHECKED : ~MIA_CHECKED));    // turn on/off

}   /* end CheckMenu() */

VOID SwitchCheck(ULONG id, ULONG *CurrentId)
{
    CheckMenu(*CurrentId, FALSE);   // uncheck prev.
    CheckMenu(id, TRUE);            // check this one
    *CurrentId = id;
}

SaveAsBitmap(VOID)
{
#define  OPEN_FILE    0x01
#define  CREATE_FILE  0x10
#define  FILE_ARCHIVE 0x20
#define  FILE_EXISTS  OPEN_FILE
#define  FILE_NOXISTS CREATE_FILE
#define  DASD_FLAG    0
#define  INHERIT      0x80
#define  WRITE_THRU   0
#define  FAIL_FLAG    0
#define  SHARE_FLAG   0x10
#define  ACCESS_FLAG  0x02
#define  FILE_ATTRIBUTE FILE_ARCHIVE

    CHAR               FileName[256];
    ULONG               Wrote0, Wrote1, Wrote2;
    ULONG               Action;
    ULONG               FileHeaderSize;
    ULONG               BmpHeaderSize;
    ULONG               RGBTableSize;
    LONG                lScansReturned;
    HFILE               FileHandle;
    APIRET              rc;
    BITMAPFILEHEADER2   FileHeader;
    PBITMAPINFO2        pbmi2InfoTable;
    PBYTE               pBuff;   // copy of bitmap


    FileHeaderSize = sizeof(BITMAPFILEHEADER2);
    BmpHeaderSize = sizeof(BITMAPINFOHEADER2);
    RGBTableSize = maxcolor1 * sizeof(RGB2);

    // build the file header
    FileHeader.usType = (USHORT) BFT_BMAP;
    FileHeader.cbSize = sizeof(BITMAPFILEHEADER2) + RGBTableSize + databytes;
    FileHeader.offBits = sizeof(BITMAPFILEHEADER2) + RGBTableSize;

    while (1)
    {
        if (GetFileName(FDS_SAVEAS_DIALOG, FileName))
           return 1; // fail or DID_CANCEL

        DosAllocMem((PPVOID)&pbmi2InfoTable,
                             sizeof(BITMAPINFOHEADER2) + RGBTableSize,
                             PAG_COMMIT | PAG_READ | PAG_WRITE);
        DosAllocMem((PPVOID)&pBuff,
                             databytes,
                             PAG_COMMIT | PAG_READ | PAG_WRITE);

        // these must be initted
        pbmi2InfoTable->cbFix = sizeof(BITMAPINFOHEADER2);
        pbmi2InfoTable->cPlanes = pbmi->cPlanes;
        pbmi2InfoTable->cBitCount = pbmi->cBitCount;
        pbmi2InfoTable->ulCompression = BCA_UNCOMP;
        pbmi2InfoTable->usReserved = 0;
        pbmi2InfoTable->usRecording = BRA_BOTTOMUP;
        pbmi2InfoTable->usRendering = BRH_NOTHALFTONED;
        pbmi2InfoTable->ulColorEncoding = BCE_RGB;
        // now put it the current RGB table
        memcpy(&pbmi2InfoTable->ulIdentifier+sizeof(ULONG), blTable, RGBTableSize);

        lScansReturned = GpiQueryBitmapBits( hpsMem,
                                             0L,         // lScanStart
                                             cyClient,   // lScans
                                             pBuff,
                                             pbmi2InfoTable);
        if (lScansReturned == 0)
          break;

        rc = DosOpen ( FileName,
                       &FileHandle,
                       &Action,
                       FileHeaderSize + RGBTableSize + databytes,
                       FILE_ATTRIBUTE,
                       OPEN_ACTION_CREATE_IF_NEW | OPEN_ACTION_FAIL_IF_EXISTS,
                       DASD_FLAG | INHERIT | SHARE_FLAG | ACCESS_FLAG,
                       0L);
        if (rc)
          break;

        rc = DosWrite( FileHandle,
                       (PVOID) (&FileHeader),
                       FileHeaderSize - BmpHeaderSize,  // cause they are in sep locations
                       &Wrote0);
        if (rc)
           break;
        rc = DosWrite( FileHandle,
                       (PVOID) pbmi2InfoTable,
                       BmpHeaderSize + RGBTableSize,  // cause they are in sep locations
                       &Wrote0);
        if (rc)
           break;


        rc = DosWrite( FileHandle,
                       pBuff,     //pBitmap
                       databytes,
                       &Wrote2);
        if (rc)
          break;


        rc = DosClose(FileHandle);

        if (rc)
          break;

        DosFreeMem(pbmi2InfoTable);
        DosFreeMem(pBuff);

        return 0; // passed

    } // end while (1)

    return 1;   // failed
}

LoadFractalBitmap(VOID)
{
#define  EABUF        0L
    CHAR              FileName[256];
    FILESTATUS3        filestatus3;
    ULONG              cBytes;
    ULONG              cbImageData;
    APIRET             rc;
    ULONG              Action;
    HFILE              FileHandle;
    SWP                winpos;
    LONG               lScansReturned;
    BITMAPINFOHEADER2  bmp;

    while(1)
    {
        if (GetFileName(FDS_OPEN_DIALOG, FileName))
               return 1; // fail or DID_CANCEL

        rc = DosOpen ( FileName,
                       &FileHandle,
                       &Action,
                       0L,
                       FILE_ATTRIBUTE,
                       OPEN_ACTION_OPEN_IF_EXISTS,
                       DASD_FLAG | INHERIT | SHARE_FLAG | ACCESS_FLAG,
                       0L);
        if (rc)
           break;
        rc = DosQueryFileInfo(FileHandle, FIL_STANDARD,
                                   (PVOID)&filestatus3, sizeof( filestatus3 ));
        // alloc memory for entire bitmap file read
        DosAllocMem((PPVOID)&pbmpfileheader2,
                             filestatus3.cbFile,
                             PAG_COMMIT | PAG_READ | PAG_WRITE);
        rc = DosRead( FileHandle, (PVOID)pbmpfileheader2, filestatus3.cbFile, &cBytes );
        if( rc != 0  ||  cBytes == 0 )
        {
           free( pbmpfileheader2 );
           DosClose( FileHandle );
           //WinPostMsg( hwndToAck, WM_NACK_FILE_READING_ERROR,
           //               (MPARAM)msg, (MPARAM)0L );
           return 1;
        }
        DosClose( FileHandle );
        // set pbmp2 to point to the BITMAPINFO2 structure within the file
        pbmp2 = (PBITMAPINFO2)(&pbmpfileheader2->bmp2);
        pBitsfile = (PBYTE)pbmpfileheader2 + pbmpfileheader2->offBits;

        cxfilebmp = pbmp2->cx;   //need these as globals
        cyfilebmp = pbmp2->cy;

        return 0;   // pass
    }
    return 1;    // fail
}

/*+--------------------------------------------------------------------------+*/
/*| ClientWindowProc - Client window procedure.                              |*/
/*+--------------------------------------------------------------------------+*/

static MRESULT EXPENTRY ClientWindowProc( HWND hwnd, ULONG msg, MPARAM mp1, MPARAM mp2 )
{
   HPS                       hps = NULLHANDLE;    /* presentation space handle      */
   int                       killer;
   int                       i;
   USHORT red,
          blue,
          green;

   static BOOL               fCapture, fShowBox;
   static POINTL             ptlStart, ptlEnd, ptlBoxStart, ptlBoxEnd;

   /* params for WinDrawBitmap */
   RECTL                     rectl;

   int                       ixl, ixr, iyb, iyt, temp;

   /* stuff for bitmap draws */
   static HDC                hdcMem;
   static HDC                hdcMemfile;
   PSZ                       pszData[4] = {"Display", NULL, NULL, NULL};
   SIZEL                     sizlPage = {0, 0};
   SIZEL                     sizlFilePage = {0, 0};
   static BITMAPINFOHEADER2  bmp;
   static HBITMAP            hbmOld;

   POINTL                    aptl[4];
   POINTL                    textat;
   POINTL                    aptflFile[4];
   LONG                      rc;

   LONG                      alData[2];
   LONG                      ix0, iy0;
   double                    cx0, cy0;

   PID                       pidowner;
   TID                       tidowner;
   ULONG                     claimcount;

   static ULONG              headerbytes;
   SWP swp;

   switch( msg )
   {
      case WM_CREATE:
          //The client window has been created but is not visible yet.
          // Initialize the window here. Also, set up device context, etc.,

          hwndMenu = WinWindowFromID( WinQueryWindow( hwnd, QW_PARENT),
                                      FID_MENU);

          hdcMem = DevOpenDC(hab, OD_MEMORY, "*", 4, (PDEVOPENDATA) pszData, NULLHANDLE);

          hpsMem = GpiCreatePS(hab, hdcMem, &sizlPage, PU_PELS | GPIT_MICRO | GPIA_ASSOC );

          DevQueryCaps(hdcMem, CAPS_COLORS, 256L, alTable);
          maxcolor1 = alTable[0];
          maxheight = (double) (maxcolor1 + maxcolor1/5);

          hps = WinGetPS(hwnd);

          GpiQueryLogColorTable(hps, 0L, 0L, maxcolor1, blTable);  /*-- what's loaded */

          headerbytes = (ULONG)sizeof(BITMAPINFOHEADER2) + maxcolor1*sizeof(RGB2);

          DosAllocMem((PPVOID)&pbmi, headerbytes, PAG_COMMIT | PAG_READ | PAG_WRITE);

          LoadColors(".\\DEFAULT.MAP", blTable, maxcolor1);

          //GpiQueryRealColors(hps, 0, 16L, maxcolor1-16L, blTable); // physical device table
          blTable[0] = 0;    // must be black
          GpiCreateLogColorTable(hps, 0L, LCOLF_CONSECRGB, 0L, maxcolor1, blTable);
          WinReleasePS(hps);

          // initialize size of fractal window in real x,y coords. also iy
          iy = 0;
          xlv = -2.5;
          xrv = 1.5;
          ybv = -1.5;
          ytv = 1.5;
          dxv = xrv - xlv;
          fractype = MANDEL;  // initial default
          strcpy(fractnamestr, MANDELZ);
          CurrentFractalChecked = IDM_MANDEL;
          CurrentMapChecked = IDM_DEFAULT;
          CurrentImageOrbitsChecked = IDM_IMAGE;
          CurrentFixChangeColorsChecked = IDM_FIXCOLORS;
          three_d = 0;
          spherical = FALSE;
          usepotential = 0;
	  pi = M_PI;
	  rpd = pi/180.0;
          alpha = 20.0;
          beta = 10.0;
          phi = 10.0;
	  sa = sin(alpha*rpd);
	  ca = cos(alpha*rpd);
	  sb = sin(beta*rpd);
	  cb = cos(beta*rpd);
          sp = sin(phi*rpd);
          cp = cos(phi*rpd);

          break;

      case WM_SIZE:
         /* Window is being resized so get new client window size  */
         hps = WinGetPS(hwnd);

         cxCold = cxClient;
         cyCold = cyClient;
         cxClient = (((LOUSHORT( mp2 )+7)/8)*8);    /* Get new client window size     */
         cyClient = (((HIUSHORT( mp2 )+7)/8)*8);    /* has to be 32-bit word alligned */

          // make cx even # of words * 4 bytes/word * height * number of color planes
          if (pBitmap != NULL)
             DosFreeMem(pBitmap);
          databytes = (cxClient+1+31)/32*4*cyClient*8;
          DosAllocMem((PPVOID)&pBitmap, databytes, PAG_COMMIT | PAG_READ | PAG_WRITE);

         // determine the device's plane/bit count format
         GpiQueryDeviceBitmapFormats(hpsMem, 2, alData);

          bmp.cbFix           = (ULONG)sizeof(BITMAPINFOHEADER2);
          bmp.cx              = cxClient;
          bmp.cy              = cyClient;
          bmp.cPlanes         = alData[0];
          bmp.cBitCount       = alData[1];
          bmp.ulCompression   = BCA_UNCOMP;
          bmp.cbImage         = cxClient;
          bmp.cxResolution    = 70;
          bmp.cyResolution    = 70;
          bmp.cclrUsed        = maxcolor1;
          bmp.cclrImportant   = 0;
          bmp.usUnits         = BRU_METRIC;
          bmp.usReserved      = 0;
          bmp.usRecording     = BRA_BOTTOMUP;
          bmp.usRendering     = BRH_NOTHALFTONED;
          bmp.cSize1          = 0;
          bmp.cSize2          = 0;
          bmp.ulColorEncoding = BCE_RGB;
          bmp.ulIdentifier    = 0;

          pbmi->cbFix         = bmp.cbFix;
          pbmi->cx            = cxClient;
          pbmi->cy            = cyClient;
          pbmi->cPlanes       = alData[0];
          pbmi->cBitCount     = alData[1];
          pbmi->ulCompression = BCA_UNCOMP;
          pbmi->cbImage       = ((cxClient+31)/32)*4;
          pbmi->cbImage       = bmp.cbImage;
          pbmi->cxResolution  = 70;
          pbmi->cyResolution  = 70;
          pbmi->cclrUsed      = bmp.cclrUsed;
          pbmi->cclrImportant = 0;
          pbmi->usUnits       = BRU_METRIC;
          pbmi->usReserved    = 0;
          pbmi->usRecording   = BRA_BOTTOMUP;
          pbmi->usRendering   = BRH_NOTHALFTONED;
          pbmi->cSize1        = 0;
          pbmi->cSize2        = 0;
          pbmi->ulColorEncoding = BCE_RGB;
          pbmi->ulIdentifier  = 0;
          for (i=0; i<maxcolor1; i++) {
             pbmi->argbColor[i].bBlue  =  blTable[i] & 0x000000ff;
             pbmi->argbColor[i].bGreen = (blTable[i] & 0x0000ff00) >> 8;
             pbmi->argbColor[i].bRed   = (blTable[i] & 0x00ff0000) >> 16;
          }

          // now create a bitmap which is compatible with display
         hbm = GpiCreateBitmap(hpsMem, &bmp, FALSE, NULL, NULL);

          // associate the bitmap and the memory presentation space
         hbmOld = GpiSetBitmap(hpsMem, hbm);
         GpiSetBitmapBits(hpsMem, 0L, cyClient, pBitmap, pbmi);
         GpiDeleteBitmap(hbmOld);


         // turn thread2 loose
         semrslt = DosQueryMutexSem(sema4, &pidowner, &tidowner, &claimcount);
         if ((tidowner == 1) && (claimcount > 0))  // does thread #1 own it?
           semrslt = DosReleaseMutexSem(sema4);

         /* turn thread3 loose */
         semrslt = DosQueryMutexSem(semsleeper, &pidowner, &tidowner, &claimcount);
         if ((tidowner == 1) && (claimcount > 0))  /* does thread #1 own it? */
           semrslt = DosReleaseMutexSem(semsleeper);

         /* turn thread4 loose */
         semrslt = DosQueryMutexSem(semcolorer, &pidowner, &tidowner, &claimcount);
         if ((tidowner == 1) && (claimcount > 0))  /* does thread #1 own it? */
           semrslt = DosReleaseMutexSem(semcolorer);

          WinReleasePS(hps);

         break;

      case WM_CLOSE:
         /* Tell the main procedure to terminate the message loop.            */

         WinPostMsg( hwnd, WM_QUIT, NULL, NULL );
         DosFreeMem((PPVOID)&pbmi);
         GpiDestroyPS(hpsMem);
         DevCloseDC(hdcMem);
         GpiDeleteBitmap(hbm);
         break;

      case WM_PAINT:
         hps = WinBeginPaint( hwnd, NULLHANDLE, NULL );
         for (i=0; i<maxcolor1; i++) {
             pbmi->argbColor[i].bBlue  =  blTable[i] & 0x000000ff;
             pbmi->argbColor[i].bGreen = (blTable[i] & 0x0000ff00) >> 8;
             pbmi->argbColor[i].bRed   = (blTable[i] & 0x00ff0000) >> 16;
         }

         aptl[0].x = 0;
         aptl[0].y = 0;
         aptl[1].x = cxClient-1;
         aptl[1].y = cyClient-1;
         aptl[2].x = 0;
         aptl[2].y = 0;
         aptl[3].x = cxCold-1;
         aptl[3].y = cyCold-1;

         // set up the file rectangle
         aptflFile[0].x = 0;
         aptflFile[0].y = 0;
         aptflFile[1].x = cxClient-1;
         aptflFile[1].y = cyClient-1;
         aptflFile[2].x = 0;
         aptflFile[2].y = 0;
         aptflFile[3].x = cxfilebmp-1;
         aptflFile[3].y = cyfilebmp-1;

         GpiSetBitmapBits(hpsMem, 0L, cyClient, pBitmap, pbmi);


         if (colorit) {
             if (hpsfile == NULLHANDLE)
                 GpiBitBlt(hps, hpsMem, 3L, aptl, ROP_SRCCOPY, BBO_IGNORE);
             else {  // we are viewing a bitmap from a file
                // blast from hpsfile (load bitmap from file)
                 GpiBitBlt(hps, hpsfile, 4L, aptflFile, ROP_SRCCOPY, BBO_IGNORE);
             }
         }
         else {
            WinQueryWindowRect(hwnd, &rectl);  // gonna draw orbits
            WinFillRect(hps, &rectl, CLR_BLACK);
         }

         // now draw the zoom box - if needed
         if (fCapture || fShowBox) {
            GpiSetMix(hps, FM_INVERT);
            GpiMove(hps, &ptlStart);
            GpiBox(hps, DRO_OUTLINE, &ptlEnd, 0L, 0L);
         }

         WinEndPaint( hps );

         break;

      case WM_COMMAND:
         // The user has chosen a menu item.  Process the selection.
         switch ( SHORT1FROMMP( mp1 ) )
         {
            case IDM_EXITPROG:
               // Post a close message.
               WinPostMsg( hwnd, WM_CLOSE, NULL, NULL );
               break;

            case IDM_RESUME:
               WinPostQueueMsg( hmqThread2, WM_USER_REPAINT, 0UL, 0UL );
               break;

            case IDM_MANDEL :
                     fractype = MANDEL;
                     SwitchCheck(IDM_MANDEL, &CurrentFractalChecked);
                     UpdateTitle(hwndFrame, szAppName, MANDELZ);
                     break;
            case IDM_LAMBDA:
                     fractype = LAMBDA;
                     SwitchCheck(IDM_LAMBDA, &CurrentFractalChecked);
                     UpdateTitle(hwndFrame, szAppName, LAMBDAZ);
                     break;
            case IDM_CMPLXNEWT:
                     fractype = CMPLXNEWT;
                     SwitchCheck(IDM_CMPLXNEWT, &CurrentFractalChecked);
                     UpdateTitle(hwndFrame, szAppName, CMPLXNEWTZ);
                     break;
            case IDM_JULIA1:
                     fractype = JULIA1;
                     SwitchCheck(IDM_JULIA1, &CurrentFractalChecked);
                     UpdateTitle(hwndFrame, szAppName, JULIAZ);
                     break;
            case IDM_CMPLXMAN:
                      fractype = COMPLEXMAND;
                      SwitchCheck(IDM_CMPLXMAN, &CurrentFractalChecked);
                      UpdateTitle(hwndFrame, szAppName, COMPLEXMANDZ);
                      break;
            case IDM_MAGNETJ1:
                      fractype = MAGNETJ1;
                      SwitchCheck(IDM_MAGNETJ1, &CurrentFractalChecked);
                      UpdateTitle(hwndFrame, szAppName, MAGNETJ1Z);
                      break;
            case IDM_TETRATION:
                      fractype = TETRATION;
                      SwitchCheck(IDM_TETRATION, &CurrentFractalChecked);
                      UpdateTitle(hwndFrame, szAppName, TETRATIONZ);
                      break;
            case IDM_SPIDER :
                      fractype = SPIDER;
                      SwitchCheck(IDM_SPIDER, &CurrentFractalChecked);
                      UpdateTitle(hwndFrame, szAppName, SPIDERZ);
                      break;
            case IDM_RESET :
                      if (hpsfile != NULLHANDLE)
                         GpiDestroyPS(hpsfile);  // make sure it's deceased
                      hpsfile = NULLHANDLE;
                      iy = 0;
                      xlv = -2.5;
                      xrv = 1.5;
                      ybv = -1.5;
                      ytv = 1.5;
                      dxv = xrv - xlv;
                      dyv = ytv - ybv;
                      showorbits = 0;
                      xperturb = 0.0;
                      yperturb = 0.0;
                      three_d = 0;
                      usepotential = 0;
                      spherical = FALSE;
                      fractype = MANDEL;   // initial default
                      SwitchCheck(IDM_MANDEL, &CurrentFractalChecked);
                      UpdateTitle(hwndFrame, szAppName, MANDELZ);
                      break;

            case IDM_SAVE :
                    if (SaveAsBitmap() )
                       DisplayMessage(hab, IDS_FAILCREATE );
                    break;

            case IDM_VIEW :
                    CheckMenu(IDM_VIEW, TRUE);
                    CheckMenu(IDM_UNVIEW, FALSE);
                    if (LoadFractalBitmap() ) {
                      DisplayMessage(hab, IDS_FAILOPEN);
                      break;
                    }
                    hdcMemfile = DevOpenDC(hab, OD_MEMORY, "*", 4,
                                          (PDEVOPENDATA) pszData, NULLHANDLE);
                    hpsfile = GpiCreatePS(hab, hdcMemfile, &sizlFilePage,
                                          PU_PELS | GPIT_MICRO | GPIA_ASSOC );
                    hbmfile = GpiCreateBitmap(hpsfile, (PBITMAPINFOHEADER2) pbmp2,
                                              0, NULL, NULL);
                    // associate the bitmap and the memory presentation space
                    GpiSetBitmap(hpsfile, hbmfile);
                    GpiSetBitmapBits(hpsfile,
                                     0L,            // scanstart
                                     pbmp2->cy,     // #of scans
                                     pBitsfile,     // the bitmap
                                     pbmp2);
                    break;

            case IDM_UNVIEW :
                    CheckMenu(IDM_VIEW, FALSE);
                    CheckMenu(IDM_UNVIEW, TRUE);
                    GpiDestroyPS(hpsfile);
                    DevCloseDC(hdcMemfile);
                    GpiDeleteBitmap(hbmfile);
                    hpsfile = NULLHANDLE;
                    DosFreeMem(pbmpfileheader2);  // allocated in LoadFractalBitmap
                    break;

            case IDM_ORBIT :
                    SwitchCheck(IDM_ORBIT, &CurrentImageOrbitsChecked);
                    showorbits = 1;
                    colorit = 0;
                    break;
            case IDM_IMAGE :
                    SwitchCheck(IDM_IMAGE, &CurrentImageOrbitsChecked);
                    showorbits = 0;
                    colorit = 1;
                    break;
            case IDM_FIXCOLORS :
                    SwitchCheck(IDM_FIXCOLORS, &CurrentFixChangeColorsChecked);
                    changecolors = 0;
                    break;
            case IDM_CHANGECOLORS :
                    SwitchCheck(IDM_CHANGECOLORS, &CurrentFixChangeColorsChecked);
                    changecolors = 1;
                    break;

            case IDM_DEFAULT :  // start of color maps
                    CheckMenu(IDM_COMPLEMENT, FALSE);
                    CheckComplementAgain = FALSE;
                    SwitchCheck(IDM_DEFAULT, &CurrentMapChecked);
                    LoadColors(".\\default.map", blTable, maxcolor1);
                    break;
            case IDM_ALTERN :
                    CheckMenu(IDM_COMPLEMENT, FALSE);
                    CheckComplementAgain = FALSE;
                    SwitchCheck(IDM_ALTERN, &CurrentMapChecked);
                    LoadColors(".\\altern.map", blTable, maxcolor1);
                    break;
            case IDM_BLUES :
                    CheckMenu(IDM_COMPLEMENT, FALSE);
                    CheckComplementAgain = FALSE;
                    SwitchCheck(IDM_BLUES, &CurrentMapChecked);
                    LoadColors(".\\blues.map", blTable, maxcolor1);
                    break;
            case IDM_CHROMA :
                    CheckMenu(IDM_COMPLEMENT, FALSE);
                    CheckComplementAgain = FALSE;
                    SwitchCheck(IDM_CHROMA, &CurrentMapChecked);
                    LoadColors(".\\chroma.map", blTable, maxcolor1);
                    break;
            case IDM_DARKGREY :
                    CheckMenu(IDM_COMPLEMENT, FALSE);
                    CheckComplementAgain = FALSE;
                    SwitchCheck(IDM_DARKGREY, &CurrentMapChecked);
                    LoadColors(".\\darkgrey.map", blTable, maxcolor1);
                    break;
            case IDM_FIRESTRM :
                    CheckMenu(IDM_COMPLEMENT, FALSE);
                    CheckComplementAgain = FALSE;
                    SwitchCheck(IDM_FIRESTRM, &CurrentMapChecked);
                    LoadColors(".\\firestrm.map", blTable, maxcolor1);
                    break;
            case IDM_GAMMA1 :
                    CheckMenu(IDM_COMPLEMENT, FALSE);
                    CheckComplementAgain = FALSE;
                    SwitchCheck(IDM_GAMMA1, &CurrentMapChecked);
                    LoadColors(".\\gamma1.map", blTable, maxcolor1);
                    break;
            case IDM_GAMMA2 :
                    CheckMenu(IDM_COMPLEMENT, FALSE);
                    CheckComplementAgain = FALSE;
                    SwitchCheck(IDM_GAMMA2,  &CurrentMapChecked);
                    LoadColors(".\\gamma2.map", blTable, maxcolor1);
                    break;
            case IDM_GREEN :
                    CheckMenu(IDM_COMPLEMENT, FALSE);
                    CheckComplementAgain = FALSE;
                    SwitchCheck(IDM_GREEN, &CurrentMapChecked);
                    LoadColors(".\\green.map", blTable, maxcolor1);
                    break;
            case IDM_GREY :
                    CheckMenu(IDM_COMPLEMENT, FALSE);
                    CheckComplementAgain = FALSE;
                    SwitchCheck(IDM_GREY, &CurrentMapChecked);
                    LoadColors(".\\grey.map", blTable, maxcolor1);
                    break;
            case IDM_LANDSCAP :
                    CheckMenu(IDM_COMPLEMENT, FALSE);
                    CheckComplementAgain = FALSE;
                    SwitchCheck(IDM_LANDSCAP, &CurrentMapChecked);
                    LoadColors(".\\landscap.map", blTable, maxcolor1);
                    break;
            case IDM_NEON :
                    CheckMenu(IDM_COMPLEMENT, FALSE);
                    CheckComplementAgain = FALSE;
                    SwitchCheck(IDM_NEON, &CurrentMapChecked);
                    LoadColors(".\\neon.map", blTable, maxcolor1);
                    break;
            case IDM_PAINTJET :
                    CheckMenu(IDM_COMPLEMENT, FALSE);
                    CheckComplementAgain = FALSE;
                    SwitchCheck(IDM_PAINTJET, &CurrentMapChecked);
                    LoadColors (".\\paintjet", blTable, maxcolor1);
                    break;
            case IDM_ROYAL :
                    CheckMenu(IDM_COMPLEMENT, FALSE);
                    CheckComplementAgain = FALSE;
                    SwitchCheck(IDM_ROYAL, &CurrentMapChecked);
                    LoadColors(".\\royal.map", blTable, maxcolor1);
                    break;
            case IDM_TOPO :
                    CheckMenu(IDM_COMPLEMENT, FALSE);
                    CheckComplementAgain = FALSE;
                    SwitchCheck(IDM_TOPO, &CurrentMapChecked);
                    LoadColors(".\\topo.map", blTable, maxcolor1);
                    break;
            case IDM_VOLCANO :
                    CheckMenu(IDM_COMPLEMENT, FALSE);
                    CheckComplementAgain = FALSE;
                    SwitchCheck(IDM_VOLCANO, &CurrentMapChecked);
                    LoadColors(".\\volcano.map", blTable, maxcolor1);
                    break;
            case IDM_RANDOM :
                    CheckMenu(IDM_COMPLEMENT, FALSE);
                    CheckComplementAgain = FALSE;
                    SwitchCheck(IDM_RANDOM, &CurrentMapChecked);
                    LoadColors(".\\random.map", blTable, maxcolor1);
                    break;
            case IDM_COMPLEMENT :
                    CheckComplementAgain ^= TRUE;
                    CheckMenu(IDM_COMPLEMENT, CheckComplementAgain);
                    for (i=1; i<maxcolor1; i++)
                      blTable[i] = (~blTable[i]) & 0xffffff;
                    break;

            case IDM_SETTINGS:
                WinDlgBox(HWND_DESKTOP, hwnd, SettingsDlg, 0, DLG_SETTINGS, 0);
                return 0;

            case IDM_HELPINSTRUCTIONS:
               /* Display the help dialog box.                                */

               WinMessageBox( HWND_DESKTOP,      /* desktop is the parent     */
                              hwnd,              /* owned by client window    */
                              INSTRUCTIONS,      /* pointer to message text   */
                              szAppName,         /* pointer to title text     */
                              1UL,               /* message box identifier    */
                              MB_OK | MB_INFORMATION | /* message box style   */
                              MB_APPLMODAL | MB_MOVEABLE );

               break;

            case IDM_HELPABOUT:
               /* Display the help dialog box.                                */

               WinDlgBox( HWND_DESKTOP,     /* make the desktop the parent    */
                          hwnd,             /* owned by client window         */
                          HelpDlgProc,      /* address of dialog procedure    */
                          0UL,              /* module handle                  */
                          IDD_HELP,         /* dialog identifier in resource  */
                          NULL );           /* initialization data            */

               break;
         }
         WinInvalidateRect(hwndClient, NULL, FALSE);
         break;


      case WM_BUTTON1DOWN:
         WinSetFocus( HWND_DESKTOP, hwnd );
         ptlStart.x = ptlEnd.x = MOUSEMSG(&msg)->x;
         ptlStart.y = ptlEnd.y = MOUSEMSG(&msg)->y;

         DrawBoxOutline(hwnd, &ptlStart, &ptlEnd);

         WinSetCapture(HWND_DESKTOP, hwnd);
         fCapture = TRUE;

         return ( WinDefWindowProc( hwnd, msg, mp1, mp2 ) );

      case WM_BUTTON1DBLCLK:
          // The user has double clicked mouse button 1 which means
          // clear to black and redraw
         iy = 0;
         memset(pBitmap, 0, databytes);
         WinInvalidateRect(hwnd, NULL, FALSE);
         break;

      case WM_BUTTON2DOWN:  // switch to Julia
         ix0 = MOUSEMSG(&msg)->x;
         iy0 = MOUSEMSG(&msg)->y;
         cy0 = ytv + (double)(iy0 - maxyp) * dyv/rmaxyp;
         cx0 = dxv * (double) ix0/rmaxxp + xlv;
         xperturb = cx0;         // these two mostly for dialog box field
         yperturb = cy0;
         c0 = complex(cx0, cy0);
         fractype = JULIACLICK;
         UpdateTitle(hwndFrame, szAppName, JULIAZ);
         SwitchCheck(IDM_JULIA1, &CurrentFractalChecked);
         xlv = -2.0;    // now start it big again
         xrv = 2.0;
         ybv = -1.5;
         ytv = 1.5;
         dxv = xrv - xlv;
         dyv = ytv - ybv;
         return (MRESULT) TRUE;

      case WM_BUTTON1UP:
         if (fCapture) {
             ptlBoxStart = ptlStart;
             ptlBoxEnd.x = MOUSEMSG(&msg)->x;
             ptlBoxEnd.y = MOUSEMSG(&msg)->y;
             WinSetCapture(HWND_DESKTOP, NULLHANDLE);   // off
             fCapture = FALSE;
             fShowBox = FALSE;
             ixl = ptlBoxStart.x;
             ixr = ptlBoxEnd.x;
             iyt = ptlBoxStart.y;
             iyb = ptlBoxEnd.y;
             if (ixl > ixr) {
               temp = ixl;
               ixl = ixr;
               ixr = temp;
             }
             if (iyb > iyt) {
               temp = iyb;
               iyb = iyt;
               iyt = temp;
             }
             // check for small box - user reject or clear box
             if (((ixl - ixr) < 8) && ((iyt - iyb) < 8)) {
                  fShowBox = FALSE;
                  fCapture = FALSE;
                  break;
             }
             dxv = xrv - xlv;
             dyv = ytv - ybv;
             rmaxxp = (double) (cxClient-1);
             rmaxyp = (double) (cyClient-1);

             xrv = dxv * (double) ixr / rmaxxp + xlv;
             xlv = dxv * (double) ixl / rmaxxp + xlv;

             ybv = ytv + ((double)iyb - rmaxyp)/rmaxyp * dyv;
             ytv = ytv + ((double)iyt - rmaxyp)/rmaxyp * dyv;
             dxv = xrv - xlv;
             dyv = ytv - ybv;
             WinInvalidateRect(hwnd, NULL, FALSE);
         }
         break;

      case WM_MOUSEMOVE:
         if (fCapture) {
             DrawBoxOutline(hwnd, &ptlStart, &ptlEnd);
             ptlEnd.x = MOUSEMSG(&msg)->x;
             ptlEnd.y = MOUSEMSG(&msg)->y;
             DrawBoxOutline(hwnd, &ptlStart, &ptlEnd);
         }
         return ( WinDefWindowProc( hwnd, msg, mp1, mp2 ) );

      case WM_DESTROY:
         /* The client window is being destroyed so tell the second thread to */
         /* stop running.                                                     */

         WinPostQueueMsg( hmqThread2, WM_USER_END_THREAD, 0UL, 0UL );

         /* Wait for the drawing thread to terminate before returning.        */

         DosWaitThread( &tidDrawing, DCWW_WAIT );

         break;
       default:
         /* For all other messages, let the default window procedure          */
         /* process them.                                                     */

         return ( WinDefWindowProc( hwnd, msg, mp1, mp2 ) );

   }
   return 0;
}
/*+--------------------------------------------------------------------------+*/
/*| BasicSettingsDlg -                                                       |*/
/*+--------------------------------------------------------------------------+*/

static MRESULT EXPENTRY BasicSettingsDlg ( HWND hwndSettings, ULONG msg, MPARAM mp1, MPARAM mp2)
{
static HWND hwndlbox;
char
     xpstr[20],
     ypstr[20],  // perturbation strings
     xlstr[20],
     xrstr[20],
     ybstr[20],
     ytstr[20],  // input/output params for four fields
     iterstr[20],
     bailoutstr[20],
     examplestr[30],
     formulastr[40];

   switch(msg)
   {
       case WM_INITDLG:
          hwndlbox = HWNDFROMMP(mp1);
          sprintf(xlstr, "%lf", xlv);
          sprintf(xrstr, "%lf", xrv);
          sprintf(ybstr, "%lf", ybv);
          sprintf(ytstr, "%lf", ytv);
          sprintf(iterstr, "%ld", maxiter);
          sprintf(bailoutstr, "%3.1lf", bailout);
          sprintf(xpstr, "%lf", xperturb);
          sprintf(ypstr, "%lf", yperturb);

          switch (fractype) {
            case MANDEL:
               strcpy(fractnamestr, MANDELZ);
               strcpy(formulastr, "z = z^2 + c");
               strcpy(examplestr, "(0,0)");
               break;
            case LAMBDA:
               strcpy(fractnamestr, LAMBDAZ);
               strcpy(formulastr, "z = c*z*(1-z)");
               strcpy(examplestr, "(0.85,0.6)");
               break;
            case CMPLXNEWT:
               strcpy(fractnamestr, CMPLXNEWTZ);
               strcpy(formulastr, "z = ((n-1)z^n+r)/(n*z^(n-1))");
               strcpy(examplestr, "(3,0)");
               break;
            case JULIA1:
               strcpy(fractnamestr, JULIAZ);
               strcpy(formulastr, "z = z*z + c");
               strcpy(examplestr, "(var,var)");
               break;
            case COMPLEXMAND:
               strcpy(fractnamestr, COMPLEXMANDZ);
               strcpy(formulastr, "z = z*z*c^(p-1)+c");
               strcpy(examplestr, "(1,0)");
               break;
            case JULIACLICK:  // switch to Julia
               strcpy(fractnamestr, JULIAZ);
               strcpy(formulastr, "z = z^2 + c");
               strcpy(examplestr, "(clk_x,clk_y)");
               break;
            case MAGNETJ1:
               strcpy(fractnamestr, MAGNETJ1Z);
               strcpy(formulastr, "(see ref. 1)");
               strcpy(examplestr, "(0.3,0.6)");
               break;
            case TETRATION:
               strcpy(fractnamestr, TETRATIONZ);
               strcpy(formulastr, "z = c^z");
               strcpy(examplestr, "(0,0) see ref");
               break;
            case SPIDER :
               strcpy(fractnamestr, SPIDERZ);
               strcpy(formulastr, "z=z^2+c;c=c/2+z");
               strcpy(examplestr, "(0,0)");
            default:
              break;
          }
          WinSetDlgItemText(hwndSettings, DLG_FRACTNAME, fractnamestr);
          WinSetDlgItemText(hwndSettings, DLG_FORMULA, formulastr);
          WinSetDlgItemText(hwndSettings, DLG_EXAMPLEPERTS, examplestr);

          WinSetDlgItemText(hwndSettings, DLG_XLEFT, xlstr);
          WinSetDlgItemText(hwndSettings, DLG_XRIGHT, xrstr);
          WinSetDlgItemText(hwndSettings, DLG_YBOTTOM, ybstr);
          WinSetDlgItemText(hwndSettings, DLG_YTOP, ytstr);
          WinSetDlgItemText(hwndSettings, DLG_REALNO, xpstr);
          WinSetDlgItemText(hwndSettings, DLG_IMAGNO, ypstr);
          WinSetDlgItemText(hwndSettings, DLG_MAXITER, iterstr);
          WinSetDlgItemText(hwndSettings, DLG_BAILOUT, bailoutstr);
          return (MRESULT)TRUE;

       case WM_COMMAND:
           switch( SHORT1FROMMP( mp1 ) ) // Extract the command value
           {
              case DID_OK:
                 WinQueryDlgItemText(hwndSettings, DLG_XLEFT, 20, xlstr);
                 WinQueryDlgItemText(hwndSettings, DLG_XRIGHT, 20, xrstr);
                 WinQueryDlgItemText(hwndSettings, DLG_YBOTTOM, 20, ybstr);
                 WinQueryDlgItemText(hwndSettings, DLG_YTOP, 20, ytstr);
                 xlv = atof(xlstr);
                 xrv = atof(xrstr);
                 ybv = atof(ybstr);
                 ytv = atof(ytstr);
                 dxv = xrv - xlv;
                 dyv = ytv - ybv;
                 WinQueryDlgItemText(hwndSettings, DLG_REALNO, 20, xpstr);
                 WinQueryDlgItemText(hwndSettings, DLG_IMAGNO, 20, ypstr);
                 xperturb = atof(xpstr);
                 yperturb = atof(ypstr);
                 WinQueryDlgItemText(hwndSettings, DLG_MAXITER, 20, iterstr);
                 WinQueryDlgItemText(hwndSettings, DLG_BAILOUT, 20, bailoutstr);
                 maxiter = atoi(iterstr);
                 bailout = atof(bailoutstr);
                 return 0;
              case DID_CANCEL:
                 return 0;
           }
           break;

       default:
           break;
   }
   mp2;  // not referenced
   return (MRESULT) 0;
}


LONG ReadSliderPosition(HWND hwnd, int Which1)
{
    return (LONG) WinSendDlgItemMsg(
                                     hwnd,
                                     Which1,
                                     SLM_QUERYSLIDERINFO,
                                     MPFROM2SHORT(SMA_SLIDERARMPOSITION, SMA_INCREMENTVALUE),
                                     NULL);
}

BOOL InitializeSlider(HWND hwnd, int Which1)
{
    USHORT index;
    LONG degrees;
    CHAR acBuffer[4];
    CHAR *cData;

    cData = ltoa(0, acBuffer, 10);
    WinSetDlgItemText(hwnd, Which1, cData);
    /*--------------------------------------
    if (!WinSendDlgItemMsg(hwnd,                this is very dense. use if desired
                           Which1,
                           SLM_SETTICKSIZE,
                           MPFROM2SHORT(SMA_SETALLTICKS, 3),
                           NULL) )
              return FALSE;
    ------------------------------------------------*/
    for (index = 0; index <= 180; index+=10){
        degrees = -90 + index;
        cData = ltoa(degrees, acBuffer, 10);
        if (!WinSendDlgItemMsg(hwnd,
                               Which1,
                               SLM_SETSCALETEXT,
                               MPFROMSHORT(index),
                               MPFROMP(cData) ) )
                  return FALSE;
        if (!WinSendDlgItemMsg(hwnd,
                               Which1,
                               SLM_SETTICKSIZE,
                               MPFROM2SHORT(index, 7),
                               NULL) )
                  return FALSE;
    }
    for (index = 5; index <= 175; index+=10){   // now set some smaller ticks
         if (!WinSendDlgItemMsg(hwnd,
                               Which1,
                               SLM_SETTICKSIZE,
                               MPFROM2SHORT(index, 4),
                               NULL) )
                  return FALSE;
    }

     switch(Which1) {
        case DLGA_ALPHA :
           index = ((SHORT) alpha + 90);
           break;
        case DLGA_BETA :
           index = ((SHORT) beta + 90);
           break;
        case DLGA_PHI :
           index = ((SHORT) phi + 90);
           break;
        default :
           break;
     }
     WinSendDlgItemMsg(         // now move the slider to alpha, beta, phi
                 hwnd,
                 Which1,
                 SLM_SETSLIDERINFO,
                 MPFROM2SHORT(SMA_SLIDERARMPOSITION, SMA_INCREMENTVALUE),
                 MPFROMSHORT(index));

    return TRUE;
}

static MRESULT EXPENTRY AdvancedSettingsDlg ( HWND hwndSettings, ULONG msg, MPARAM mp1, MPARAM mp2)
{
static HWND hwndlbox;
char
     maxhtstr[20],
     insidecolorstr[20],
     bailoutstr[20];

LONG
     ulalpha,
     ulbeta,
     ulphi;

   switch(msg)
   {
       case WM_INITDLG:
          hwndlbox = HWNDFROMMP(mp1);
          sprintf(maxhtstr, "%4.1lf", maxheight);
          sprintf(insidecolorstr, "%3ld", insidecolor);
          sprintf(bailoutstr, "%6.1lf", bailout);
          WinSetDlgItemText(hwndSettings, DLGA_MAXHT, maxhtstr);
          WinSetDlgItemText(hwndSettings, DLGA_INSIDECOLOR, insidecolorstr);

          WinCheckButton(hwndSettings, DLGA_THREED, three_d);
          WinCheckButton(hwndSettings, DLGA_USEPOTENTIAL, usepotential);

          if (autoscale) {
             WinCheckButton(hwndSettings, DLG_AUTOYES, TRUE);
             WinCheckButton(hwndSettings, DLG_AUTONO, FALSE);
          }
          else {
             WinCheckButton(hwndSettings, DLG_AUTOYES, FALSE);
             WinCheckButton(hwndSettings, DLG_AUTONO, TRUE);
          }

          InitializeSlider(hwndSettings, DLGA_ALPHA);
          InitializeSlider(hwndSettings, DLGA_BETA);
          InitializeSlider(hwndSettings, DLGA_PHI);

          return (MRESULT)0;
      case WM_COMMAND:
           switch( SHORT1FROMMP( mp1 ) ) // Extract the command value
           {
              case DID_OK:
                 ulalpha = ReadSliderPosition(hwndSettings, DLGA_ALPHA);
                 alpha = (double) (ulalpha - 90);
                 ulbeta = ReadSliderPosition(hwndSettings, DLGA_BETA);
                 beta = (double) (ulbeta - 90);
                 ulphi = ReadSliderPosition(hwndSettings, DLGA_PHI);
                 phi = (double) (ulphi - 90);
                 three_d = WinQueryButtonCheckstate(hwndSettings, DLGA_THREED);
                 usepotential = WinQueryButtonCheckstate(hwndSettings, DLGA_USEPOTENTIAL);
                 autoscale = WinQueryButtonCheckstate(hwndSettings, DLG_AUTOYES);
                 WinQueryDlgItemText(hwndSettings, DLGA_MAXHT, 20, maxhtstr);
                 WinQueryDlgItemText(hwndSettings, DLGA_INSIDECOLOR, 20, insidecolorstr);
                 maxheight = atof(maxhtstr);
                 insidecolor = atoi(insidecolorstr);
                 return 0;
              case DID_CANCEL:
                 return 0;
           }
           break;

       default:
           break;
   }
   mp2; // not referenced
   return (MRESULT) 0;
}

static MRESULT EXPENTRY SpecialSettingsDlg(HWND hwndSettings, ULONG msg, MPARAM mp1, MPARAM mp2)
{
static HWND hwndlbox;
char
    radiusstr[20];

    switch(msg)
   {
       case WM_INITDLG :
          hwndlbox = HWNDFROMMP(mp1);

          if (sphere_covers_180) {
             WinCheckButton(hwndSettings, DLG_360, FALSE);
             WinCheckButton(hwndSettings, DLG_180, TRUE);
          }
          else {
             WinCheckButton(hwndSettings, DLG_180, FALSE);
             WinCheckButton(hwndSettings, DLG_360, TRUE);
          }
          sprintf(radiusstr, "%6.3lg", sphererad);
          WinSetDlgItemText(hwndSettings, DLG_SPHERERAD, radiusstr);

          WinCheckButton(hwndSettings, DLG_SPHERICAL, spherical);

          return (MRESULT) 0;
       case WM_COMMAND :
          switch(SHORT1FROMMP(mp1))
          {
              case DID_OK:
                  WinQueryDlgItemText(hwndSettings, DLG_SPHERERAD, 20, radiusstr);
                  sphererad = atof(radiusstr);
                  sphere_covers_180 = WinQueryButtonCheckstate(hwndSettings, DLG_180);
                  spherical = WinQueryButtonCheckstate(hwndSettings, DLG_SPHERICAL);
                  return 0;
              case DID_CANCEL :
                  return 0;
          }
          break;

       default :
          break;
   }
   mp2;   // not referenced
   return (MRESULT) 0;
}

/*+--------------------------------------------------------------------------+*/
/*|  SettingsDlg - Main Settings dialog procedure                            |*/
/*+--------------------------------------------------------------------------+*/

static MRESULT EXPENTRY SettingsDlg(HWND hwndSettings, ULONG msg, MPARAM mp1, MPARAM mp2)
{
int
     corner;   //corners of zoom box. there are 8 in 3-d space
double
     xpc,
     ypc,
     zpc,
     znc,
     zxc;  // stuff to rotate the zoom box if 3-D

   switch(msg)

   {
       case WM_INITDLG:
          BasicSettingsDlg(hwndSettings, msg, mp1, mp2);
          AdvancedSettingsDlg(hwndSettings, msg, mp1, mp2);
          SpecialSettingsDlg(hwndSettings, msg, mp1, mp2);
          break;

       case WM_COMMAND:
           switch( SHORT1FROMMP( mp1 ) ) // Extract the command value
           {
              case DID_OK:
                 BasicSettingsDlg(hwndSettings, msg, mp1, mp2);
                 AdvancedSettingsDlg(hwndSettings, msg, mp1, mp2);
                 SpecialSettingsDlg(hwndSettings, msg, mp1, mp2);
                 if (three_d) { // rotate the zoom box
	            sa = sin(alpha*rpd);
	            ca = cos(alpha*rpd);
	            sb = sin(beta*rpd);
	            cb = cos(beta*rpd);
                    sp = sin(phi*rpd);
                    cp = cos(phi*rpd);
                    if (!spherical)  {
                        xlp = 1.0e7;
                        xrp = -xlp;
                        ybp = xlp;
                        ytp = xrp;
                        znc = 0.0;                    // min y
                        if (usepotential) {
                           //zxc = potential/maxheight; // Oh boy, need to estimate this
                           zxc = 2.0*bailout/maxheight;
                        }
                        else
                           zxc = (double) maxcolor1/maxheight;
                        zbp = xlp;
                        ztp = -zbp;
                        for (corner=1; corner<=8; corner++) {
                           switch (corner)   // go through 8 corners of 3-d zoom box
                           {
                              case 1:
                                 rotpoint(xlv, ybv, znc, &xpc, &ypc, &zpc);
                                 break;
                              case 2:
                                 rotpoint(xlv, ybv, zxc, &xpc, &ypc, &zpc);
                                 break;
                              case 3:
                                 rotpoint(xrv, ybv, zxc, &xpc, &ypc, &zpc);
                                 break;
                              case 4:
                                 rotpoint(xrv, ybv, znc, &xpc, &ypc, &zpc);
                                 break;
                              case 5:
                                 rotpoint(xlv, ytv, znc, &xpc, &ypc, &zpc);
                                 break;
                              case 6:
                                 rotpoint(xlv, ytv, zxc, &xpc, &ypc, &zpc);
                                 break;
                              case 7:
                                 rotpoint(xrv, ytv, zxc, &xpc, &ypc, &zpc);
                                 break;
                              case 8:
                                 rotpoint(xrv, ytv, znc, &xpc, &ypc, &zpc);
                              default :
                                 break;
                           } // end switch
                           xlp = min(xlp, xpc);
                           xrp = max(xrp, xpc);
                           ybp = min(ybp, ypc);
                           ytp = max(ytp, ypc);
                           zbp = min(zbp, zpc);
                           ztp = max(ztp, zpc);
                        } // end for
                        dxvp = xrp - xlp;  // we now have the top, bottom, left, right pts
                        dyvp = ytp - ybp;  // of the rotated zoom box. Get lengths
                        dzvp = ztp - zbp;
                        //iy = 0;            // need a re-draw or ray-trace won't work
                        //memset(pBitmap, 0, databytes);
                    } // end (!spherical)
                    else // it is spherical
                    {
                        dxv = xrv - xlv;
                        sphererad = dxv/(2.0*pi);   // dxv is the circumference
                        if (sphere_covers_180)
                          sphererad *= 2.0;
                        ybps = -1.3*sphererad; // arbitrarily add 30 percent margin
                        ytps = 1.3*sphererad;
                        zbps = ybps;
                        ztps = ytps;
                        dyvps = ytps - ybps;
                        dzvps = ztps - zbps;
                        //  now make the sphere round
                        dxvps = dyvps * rmaxxp/rmaxyp;
                        xrps = dxvps/2.0;
                        xlps = -xrps;
                        //iy = 0;            // re-draw
                        //memset(pBitmap, 0, databytes);  don't think I like this
                    } // end else (spherical)
                 } // end if 3-d
                 WinDismissDlg(hwndSettings, TRUE);
                 return 0;
              case DID_CANCEL:
                 WinDismissDlg(hwndSettings, TRUE);
                 return 0;
              default :
                 break;
           }

       default:
           return WinDefDlgProc(hwndSettings, msg, mp1, mp2);
   }
   return (MRESULT) 0;
}


/*+--------------------------------------------------------------------------+*/
/*| HelpDlgProc - Help Dialog Procedure                                      |*/
/*+--------------------------------------------------------------------------+*/

static MRESULT EXPENTRY HelpDlgProc( HWND hwndDlg, ULONG msg, MPARAM mp1, MPARAM mp2 )
{
   switch( msg )
   {
      case WM_COMMAND:
         switch( SHORT1FROMMP( mp1 ) )      /* Extract the command value      */
         {
            case DID_OK:                    /* The Enter pushbutton or key    */
               WinDismissDlg( hwndDlg, TRUE );
               break;

            default:
               break;
         }
         break;

      default:
         return WinDefDlgProc( hwndDlg, msg, mp1, mp2 );
   }
   return NULL;
}


/*+--------------------------------------------------------------------------+*/
/*| DisplayMessage - display an error message in a message box.              |*/
/*+--------------------------------------------------------------------------+*/

static void DisplayMessage( HAB hab, ULONG ulStringNum )
{
   char szTemp[ STRING_LENGTH ];

   WinLoadString( hab, 0UL, ulStringNum, STRING_LENGTH, szTemp );

   WinAlarm( HWND_DESKTOP,                  /* desktop window handle          */
             WA_ERROR );                    /* type of alarm                  */

   WinMessageBox( HWND_DESKTOP,             /* parent window handle           */
                  HWND_DESKTOP,             /* owner window handle            */
                  szTemp,                   /* pointer to message text        */
                  szAppName,                /* pointer to title text          */
                  MSG_BOX_ID,               /* message box identifier         */
                  MB_OK | MB_ERROR |        /* message box style              */
                  MB_SYSTEMMODAL );

   return;
}

