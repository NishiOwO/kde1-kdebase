/* $XConsortium: server.c,v 1.19 94/04/17 20:03:44 hersh Exp $ */
/* $XFree86: xc/programs/xdm/server.c,v 3.1 1994/06/28 12:32:39 dawes Exp $ */
/* $Id: server.c,v 1.4 1998/11/27 13:59:32 steffen Exp $ */
/*

Copyright (c) 1988  X Consortium

Permission is hereby granted, free of charge, to any person obtaining
a copy of this software and associated documentation files (the
"Software"), to deal in the Software without restriction, including
without limitation the rights to use, copy, modify, merge, publish,
distribute, sublicense, and/or sell copies of the Software, and to
permit persons to whom the Software is furnished to do so, subject to
the following conditions:

The above copyright notice and this permission notice shall be included
in all copies or substantial portions of the Software.

THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS
OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF
MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT.
IN NO EVENT SHALL THE X CONSORTIUM BE LIABLE FOR ANY CLAIM, DAMAGES OR
OTHER LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE,
ARISING FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR
OTHER DEALINGS IN THE SOFTWARE.

Except as contained in this notice, the name of the X Consortium shall
not be used in advertising or otherwise to promote the sale, use or
other dealings in this Software without prior written authorization
from the X Consortium.

*/

/*
 * xdm - display manager daemon
 * Author:  Keith Packard, MIT X Consortium
 *
 * server.c - manage the X server
 */

#ifdef HAVE_CONFIG_H
#include "../config.h"
#endif

# include	"dm.h"
# include	<X11/Xlib.h>
# include	<X11/Xos.h>
# include	<stdio.h>
# include	<signal.h>
# include	<errno.h>
# include	<sys/socket.h>

#ifdef MINIX
#include <sys/ioctl.h>
#include <net/gen/in.h>
#include <net/gen/tcp.h>
#include <net/gen/tcp_io.h>
#endif

static int receivedUsr1;

#ifdef X_NOT_STDC_ENV
extern int errno;
#endif

static int serverPause ();

static Display	*dpy;

/* ARGSUSED */
static SIGVAL
CatchUsr1 (n)
    int n;
{
#ifdef SIGNALS_RESET_WHEN_CAUGHT
    (void) Signal (SIGUSR1, CatchUsr1);
#endif
    Debug ("display manager caught SIGUSR1\n");
    ++receivedUsr1;
}

extern void CleanUpChild();

int StartServerOnce (d)
struct display	*d;
{
    char	**f;
    char	**argv;
    char	arg[1024];
    char	**parseArgs ();
    int		pid;

    Debug ("StartServer for %s\n", d->name);
    receivedUsr1 = 0;
    (void) Signal (SIGUSR1, CatchUsr1);
    argv = d->argv;
    switch (pid = fork ()) {
    case 0:
	CleanUpChild ();
	if (d->authFile) {
	    sprintf (arg, "-auth %s", d->authFile);
	    argv = parseArgs (argv, arg);
	}
	if (!argv) {
	    LogError ("StartServer: no arguments\n");
	    sleep ((unsigned) d->openDelay);
	    exit (UNMANAGE_DISPLAY);
	}
	for (f = argv; *f; f++)
	    Debug ("'%s' ", *f);
	Debug ("\n");
	/*
	 * give the server SIGUSR1 ignored,
	 * it will notice that and send SIGUSR1
	 * when ready
	 */
	(void) Signal (SIGUSR1, SIG_IGN);
	(void) execv (argv[0], argv);
	LogError ("server %s cannot be executed\n",
			argv[0]);
	sleep ((unsigned) d->openDelay);
	exit (REMANAGE_DISPLAY);
    case -1:
	LogError ("fork failed, sleeping\n");
	return 0;
    default:
	break;
    }
    Debug ("Server Started %d\n", pid);
    d->serverPid = pid;
    if (serverPause ((unsigned) d->openDelay, pid))
	return FALSE;
    return TRUE;
}

int StartServer (d)
struct display *d;
{
    int	i;
    int	ret = FALSE;

    i = 0;
    while (d->serverAttempts == 0 || i < d->serverAttempts)
    {
	if ((ret = StartServerOnce (d)) == TRUE)
	    break;
	sleep (d->openDelay);
	i++;
    }
    return ret;
}

/*
 * sleep for t seconds, return 1 if the server is dead when
 * the sleep finishes, 0 else
 */

static Jmp_buf	pauseAbort;
static int	serverPauseRet;

/* ARGSUSED */
static SIGVAL
serverPauseAbort (n)
    int n;
{
    Longjmp (pauseAbort, 1);
}

/* ARGSUSED */
static SIGVAL
serverPauseUsr1 (n)
    int n;
{
    Debug ("display manager paused til SIGUSR1\n");
    ++receivedUsr1;
    Longjmp (pauseAbort, 1);
}

static
int serverPause (t, serverPid)
unsigned    t;
int	    serverPid;
{
    int		pid;

    serverPauseRet = 0;
    if (!Setjmp (pauseAbort)) {
	(void) Signal (SIGALRM, serverPauseAbort);
	(void) Signal (SIGUSR1, serverPauseUsr1);
#ifdef SYSV
	if (receivedUsr1)
	    (void) alarm ((unsigned) 1);
	else
	    (void) alarm (t);
#else
	if (!receivedUsr1)
	    (void) alarm (t);
	else
	    Debug ("Already received USR1\n");
#endif
	for (;;) {
#if defined(SYSV) && defined(X_NOT_POSIX)
	    pid = wait ((waitType *) 0);
#else
	    if (!receivedUsr1)
		pid = wait ((waitType *) 0);
	    else
#ifndef X_NOT_POSIX
		pid = waitpid (-1, (int *) 0, WNOHANG);
#else
		pid = wait3 ((waitType *) 0, WNOHANG,
			     (struct rusage *) 0);
#endif /* X_NOT_POSIX */
#endif /* SYSV */
	    if ( (pid == serverPid ||
		pid == -1) && errno == ECHILD)
	    {
		Debug ("Server dead\n");
		serverPauseRet = 1;
		break;
	    }
#if !defined(SYSV) || !defined(X_NOT_POSIX)
	    if (pid == 0) {
		Debug ("Server alive and kicking\n");
		break;
	    }
#endif
	}
    }
    (void) alarm ((unsigned) 0);
    (void) Signal (SIGALRM, SIG_DFL);
    (void) Signal (SIGUSR1, CatchUsr1);
    if (serverPauseRet) {
	Debug ("Server died\n");
	LogError ("server unexpectedly died\n");
    }
    return serverPauseRet;
}


/*
 * this code is complicated by some TCP failings.  On
 * many systems, the connect will occasionally hang forever,
 * this trouble is avoided by setting up a timeout to Longjmp
 * out of the connect (possibly leaving piles of garbage around
 * inside Xlib) and give up, terminating the server.
 */

static Jmp_buf	openAbort;

/* ARGSUSED */
static SIGVAL
abortOpen (n)
    int n;
{
	Longjmp (openAbort, 1);
}

#ifdef XDMCP

#ifdef STREAMSCONN
#include <tiuser.h>
#endif

static
void GetRemoteAddress (d, fd)
    struct display  *d;
    int		    fd;
{
    char    buf[512];
    KSIZE_T    len = sizeof (buf);
#ifdef STREAMSCONN
    struct netbuf	netb;
#endif
#ifdef MINIX
    nwio_tcpconf_t tcpconf;
#endif

    if (d->peer)
	free ((char *) d->peer);
#ifdef STREAMSCONN
    netb.maxlen = sizeof(buf);
    netb.buf = buf;
    t_getname(fd, &netb, REMOTENAME);
    len = 8;
    /* lucky for us, t_getname returns something that looks like a sockaddr */
#else
#ifdef MINIX
    if (ioctl(fd, NWIOGTCPCONF, &tcpconf) == -1)
    {
    	LogError("NWIOGTCPCONF failed: %s\n", strerror(errno));
    	len= 0;
    }
    else
    {
    	struct sockaddr_in *sinp;

    	sinp= (struct sockaddr_in *)buf;
    	len= sizeof(*sinp);
    	sinp->sin_family= AF_INET;
    	sinp->sin_port= tcpconf.nwtc_remport;
    	sinp->sin_addr.s_addr= tcpconf.nwtc_remaddr;
    }
#else
    getpeername (fd, (struct sockaddr *) buf, &len);
#endif
#endif
    d->peerlen = 0;
    if (len)
    {
	d->peer = (XdmcpNetaddr) malloc (len);
	if (d->peer)
	{
	    memmove( (char *) d->peer, buf, len);
	    d->peerlen = len;
	}
    }
    Debug ("Got remote address %s %d\n", d->name, d->peerlen);
}

#endif /* XDMCP */

static int
openErrorHandler (dpy)
    Display *dpy;
{
    LogError ("IO Error in XOpenDisplay\n");
    exit (OPENFAILED_DISPLAY);
}

extern int RegisterCloseOnFork( int fd );

int
WaitForServer (d)
    struct display  *d;
{
    /* volatile added /stefh */
    volatile int i;

    for (i = 0; i < (d->openRepeat > 0 ? d->openRepeat : 1); i++) {
    	(void) Signal (SIGALRM, abortOpen);
    	(void) alarm ((unsigned) d->openTimeout);
    	if (!Setjmp (openAbort)) {
	    Debug ("Before XOpenDisplay(%s)\n", d->name);
	    errno = 0;
	    (void) XSetIOErrorHandler (openErrorHandler);
	    dpy = XOpenDisplay (d->name);
#ifdef STREAMSCONN
	    {
		/* For some reason, the next XOpenDisplay we do is
		   going to fail, so we might as well get that out
		   of the way.  There is something broken here. */
		Display *bogusDpy = XOpenDisplay (d->name);
		Debug ("bogus XOpenDisplay %s\n",
		       bogusDpy ? "succeeded" : "failed");
		if (bogusDpy) XCloseDisplay(bogusDpy); /* just in case */
	    }
#endif
	    (void) alarm ((unsigned) 0);
	    (void) Signal (SIGALRM, SIG_DFL);
	    (void) XSetIOErrorHandler ((int (*)()) 0);
	    Debug ("After XOpenDisplay(%s)\n", d->name);
	    if (dpy) {
#ifdef XDMCP
	    	if (d->displayType.location == Foreign)
		    GetRemoteAddress (d, ConnectionNumber (dpy));
#endif
	    	RegisterCloseOnFork (ConnectionNumber (dpy));
		(void) fcntl (ConnectionNumber (dpy), F_SETFD, 0);
	    	return 1;
	    } else {
	    	Debug ("OpenDisplay failed %d (%s) on \"%s\"\n",
		       errno, strerror (errno), d->name);
	    }
	    Debug ("waiting for server to start %d\n", i);
	    sleep ((unsigned) d->openDelay);
    	} else {
	    Debug ("hung in open, aborting\n");
	    LogError ("Hung in XOpenDisplay(%s), aborting\n", d->name);
	    (void) Signal (SIGALRM, SIG_DFL);
	    break;
    	}
    }
    Debug ("giving up on server\n");
    LogError ("server open failed for %s, giving up\n", d->name);
    return 0;
}

extern void pseudoReset( Display *dpy );

void ResetServer (d)
    struct display  *d;
{
    if (dpy && d->displayType.origin != FromXDMCP)
	pseudoReset (dpy);
}

static Jmp_buf	pingTime;

static void
PingLost ()
{
    Longjmp (pingTime, 1);
}

/* ARGSUSED */
static int
PingLostIOErr (dpy)
    Display *dpy;
{
    PingLost();
    return 0;
}

/* ARGSUSED */
static SIGVAL
PingLostSig (n)
    int n;
{
    PingLost();
}

int PingServer (d, alternateDpy)
    struct display  *d;
    Display	    *alternateDpy;
{
    int	    (*oldError)();
    SIGVAL  (*oldSig)();
    int	    oldAlarm;

    if (!alternateDpy)
	alternateDpy = dpy;
    oldError = XSetIOErrorHandler ((void *)PingLostIOErr);
    oldAlarm = alarm (0);
    oldSig = Signal (SIGALRM, PingLostSig);
    (void) alarm (d->pingTimeout * 60);
    if (!Setjmp (pingTime))
    {
	Debug ("Ping server\n");
	XSync (alternateDpy, 0);
    }
    else
    {
	Debug ("Server dead\n");
	(void) alarm (0);
	(void) Signal (SIGALRM, SIG_DFL);
	XSetIOErrorHandler (oldError);
	return 0;
    }
    (void) alarm (0);
    (void) Signal (SIGALRM, oldSig);
    (void) alarm (oldAlarm);
    Debug ("Server alive\n");
    XSetIOErrorHandler (oldError);
    return 1;
}
