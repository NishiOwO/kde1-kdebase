#include "manage.h"
#include "subprotocol.h"
#include "http.h"
#include "file.h"
#include "ftp.h"
#include "tar.h"
#include "cgi.h"
#include "gzip.h"
#include "icon.h"
#include "ftp_proxy.h"

extern int revmatch(const char *host, const char *nplist);

int ProtocolSupported(const char *url)
{
    // Iterate until we saw every subprotocol
    KURL u( url );
    do
    {
	// Get the right most subprotocol
	QString sub( u.nestedURL() );
	if ( !HaveProto( sub ) )
	    return FALSE;
	// Is there something left ?
	if ( u.hasSubProtocol() )
	{
	    QString parent( u.parentURL() );
	    u = parent.data();
	}
    } while ( u.hasSubProtocol() );
    
    return TRUE;
}

int HaveProto(const char *url)
{
	// Something on the local file system ?
	if( url[0] == '/' ) return 1;
	if( strncmp( url, "ftp:", 4 ) == 0) return 1;
	if( strncmp( url, "tar:", 4 ) == 0) return 1;
	if( strncmp( url, "file:", 5 ) == 0) return 1;
	if( strncmp( url, "http:", 5 ) == 0) return 1;
	if( strncmp( url, "cgi:", 4 ) == 0) return 1;
	if( strncmp( url, "gzip:", 5 ) == 0) return 1;
	if( strncmp( url, "icon:", 5 ) == 0) return 1;
	return(0);
}

const char *TopLevelURL(const char *url)
{
	const char *lasturl;
	if( ( lasturl = strrchr(url,'#') ) ) return( lasturl+1 );
	return( url );
}

KProtocol *CreateProtocol(const char *url)
{
    KURL u( url );
    if ( u.isMalformed() )
	return NULL;

    bool do_proxy = false;
    QString ftp_proxy = getenv("ftp_proxy");
    if ( ftp_proxy.isEmpty() )
	ftp_proxy = getenv("FTP_PROXY");
    if ( !ftp_proxy.isEmpty() )
	do_proxy = true;
    
    QString lasturl( u.nestedURL() );

    char *no_proxy = getenv("no_proxy");
    if ( no_proxy != 0L && do_proxy )
    {
	do_proxy = !revmatch( u.host(), no_proxy );
    }

    // A Hack. Does allow only one subprotocol.
    if( lasturl[0] == '/' ) return(new KProtocolFILE);
    if( strncmp( lasturl, "file:", 5 ) == 0 ) return( new KProtocolFILE );
    if( strncmp( lasturl, "http:", 5 ) == 0 ) return( new KProtocolHTTP );
    // if( strncmp( lasturl, "ftp:", 4 ) == 0 ) return( new KProtocolFTP );
    if( strncmp( lasturl, "ftp:", 4 ) == 0 )
    {
	if ( do_proxy )
	    return( new KProtocolProxyFTP );
	else
	    return( new KProtocolFTP );
    }     
    if( strncmp( lasturl, "cgi:", 4 ) == 0 ) return( new KProtocolCGI );
    if( strncmp( lasturl, "icon:", 4 ) == 0 ) return( new KProtocolICON );
    if( strncmp( lasturl, "gzip:", 5 ) == 0 )
    {
	KSubProtocol *sub = new KProtocolGZIP;
	sub->InitSubProtocol( u.parentURL() );
	
	return(sub);
    }
    if( strncmp( lasturl, "tar:", 4 ) == 0 )
    {
	KSubProtocol *sub = new KProtocolTAR;
	sub->InitSubProtocol( u.parentURL() );
	
	return(sub);
    }
    return NULL;
}

