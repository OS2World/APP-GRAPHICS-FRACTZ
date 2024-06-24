
/*+--------------------------------------------------------------------------+*/
/*| Menu item identifiers.                                                   |*/
/*+--------------------------------------------------------------------------+*/

#define DLG_ADVANCED	     3
#define IDM_EXIT             257
#define IDM_EXITPROG         258
#define IDM_RESUME           259
#define IDM_RESET           400
#define IDM_SAVE            410
#define IDM_VIEW            420
#define IDM_UNVIEW          430
#define IDM_FRACTYPE        2000
#define IDM_MANDEL          2001
#define IDM_LAMBDA          2002
#define IDM_CMPLXNEWT       2003
#define IDM_JULIA1          2004
#define IDM_CMPLXMAN        2005
#define IDM_MAGNETJ1        2007
#define IDM_TETRATION       2008
#define IDM_SPIDER          2009

#define IDM_DISPLAY         1001
#define IDM_ORBIT           1002
#define IDM_IMAGE           1003
#define IDM_FIXCOLORS       1101
#define IDM_CHANGECOLORS    1102

#define IDM_SETUP           1200

#define IDM_SETTINGS        1300

#define IDM_MAP             1400
#define IDM_DEFAULT         1410
#define IDM_ALTERN          1415
#define IDM_BLUES           1420
#define IDM_CHROMA          1430
#define IDM_DARKGREY        1435
#define IDM_FIRESTRM        1440
#define IDM_GAMMA1          1445
#define IDM_GAMMA2          1446
#define IDM_GREEN           1450
#define IDM_GREY            1455
#define IDM_LANDSCAP        1457
#define IDM_NEON            1460
#define IDM_PAINTJET        1465
#define IDM_ROYAL           1470
#define IDM_TOPO            1480
#define IDM_VOLCANO         1490
#define IDM_RANDOM          1498
#define IDM_COMPLEMENT      1499

#define IDD_HELP             260
#define IDM_HELP             261
#define IDM_HELPINSTRUCTIONS 262
#define IDM_HELPABOUT        263

/*+--------------------------------------------------------------------------+*/
/*| String identifiers.                                                      |*/
/*+--------------------------------------------------------------------------+*/

#define IDS_APPNAME          0
#define IDS_NOREGISTER       1
#define IDS_NOSTDWINDOWS     2
#define IDS_NOTHREAD         3
#define IDS_SEMERROR         4
#define IDS_FAILOPEN         5
#define IDS_FAILCREATE       6

/*+--------------------------------------------------------------------------+*/
/*| Resource identifiers.                                                    |*/
/*+--------------------------------------------------------------------------+*/

#define ID_NULL              9999
#define ID_WINDOW            256

#define  DLG_SETTINGS	20
#define  DLG_REALTITLE  101
#define  DLG_IMAGTITLE  102
#define  DLG_REALNO     103
#define  DLG_IMAGNO     104
#define  DLG_XLEFT      201
#define  DLG_XRIGHT     202
#define  DLG_YBOTTOM    203
#define  DLG_YTOP       204
#define  DLG_MAXITER    301
#define  DLG_BAILOUT    302
#define  DLG_FRACTNAME  400
#define  DLG_FORMULA    401
#define  DLG_EXAMPLEPERTS  402


#define  DLGA_THREED       521
#define  DLGA_USEPOTENTIAL 522
#define  DLGA_MAXHT        523
#define  DLGA_ALPHA        524
#define  DLGA_BETA         525
#define  DLGA_PHI          526
#define  DLGA_INSIDECOLOR  527
#define  DLG_AUTOSCALE     529
#define  DLG_AUTOYES       530
#define  DLG_AUTONO        531

#define DLG_SPHERICAL	   631
#define DLG_180	           632
#define DLG_360	           633
#define DLG_SPHERERAD      634

/*+--------------------------------------------------------------------------+*/
/*| Macro constants.                                                         |*/
/*+--------------------------------------------------------------------------+*/

#define INSTRUCTIONS "LMB down and drag to form zoom box, LMB double click restarts.  RMB click starts a JULIA at the mouse (x,y)."

#define WM_USER_END_THREAD WM_USER+1  // paint thread end message
#define WM_USER_REPAINT    WM_USER+2  // paint thread repaint message

