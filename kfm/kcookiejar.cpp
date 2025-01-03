/*
    This file is part of the KDE File Manager

    Copyright (C) 1998 Waldo Bastian (bastian@kde.org)

    This library is free software; you can redistribute it and/or
    modify it under the terms of the GNU General Public License 
    as published by the Free Software Foundation; either version 2 
    of the License, or (at your option) any later version.

    This software is distributed in the hope that it will be useful,
    but WITHOUT ANY WARRANTY; without even the implied warranty of
    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
    General Public License for more details.

    You should have received a copy of the GNU General Public License
    along with this library; see the file COPYING. If not, write to
    the Free Software Foundation, Inc., 59 Temple Place - Suite 330,
    Boston, MA 02111-1307, USA.
*/
//----------------------------------------------------------------------------
//
// KDE File Manager -- HTTP Cookies
// $Id: kcookiejar.cpp,v 1.13 1998/12/20 22:46:20 waba Exp $

//
// The cookie protocol is a mess. RFC2109 is a joke since nobody seems to 
// use it. Apart from that it is badly written. 
// We try to implement Netscape Cookies and try to behave us according to
// RFC2109 as much as we can.
//

#ifdef HAVE_CONFIG_H
#include <config.h>
#endif
#include <sys/types.h>
#include <sys/stat.h>
#ifdef HAVE_SYS_PARAM_H
#include <sys/param.h>
#endif
#include <fcntl.h>
#include <unistd.h>
#include <stdio.h>
#include <string.h>
#include <ctype.h>
#include <stdlib.h>
#include <time.h>

#include <qstring.h>
#include <qstrlist.h>
#include <qlist.h>
#include <qdict.h>

#include <kurl.h>
#include <kconfig.h>

#include "kcookiejar.h"

KCookieJar *cookiejar=0;

#define READ_BUFFER_SIZE 8192

// KCookie
///////////////////////////////////////////////////////////////////////////

//
// Cookie constructor
//
KCookie::KCookie(const char *_host, const char *_domain, const char *_path,
                 const char *_name, const char *_value, time_t _expireDate,
                 int _protocolVersion) :
       host(_host), 
       domain(_domain), 
       path(_path), 
       name(_name),
       value(_value),
       expireDate(_expireDate),
       protocolVersion(_protocolVersion)
{ 
    nextCookie = 0;
}

//
// Checks if a cookie has been expired
//
bool    KCookie::isExpired(time_t currentDate)
{
    return (expireDate != 0) && (expireDate < currentDate);
}

//
// Returns a string for a HTTP-header
//
QString KCookie::getCookieStr(void)
{
    QString result;
    
    if (protocolVersion != 0)
    {
        result.sprintf("$Version=\"%d\"; ", protocolVersion);
        result += name + "=\"" + value + "\"";
        if (!path.isEmpty())
            result += "; $Path=\""+ path + "\"";
        if (!domain.isEmpty())
            result += "; $Domain=\""+ domain + "\"";
    }
    else
    {
        result = name.data();
        result = result + "=" + value;
    }    
    return result;
}

// KCookieList
///////////////////////////////////////////////////////////////////////////

int KCookieList::compareItems( KCookie * item1, KCookie * item2)
{
    int pathLen1 = item1->path.length();
    int pathLen2 = item2->path.length();
    if (pathLen1 > pathLen2)
        return 1;
    if (pathLen1 < pathLen2)
        return -1;
    return 0;
}
     

static time_t parseExpire(const char *expireDate);

// KCookieJar
///////////////////////////////////////////////////////////////////////////

//
// Constructs a new cookie jar
//
// One jar should be enough for all cookies.
// 
KCookieJar::KCookieJar()
{
    cookieDomains.setAutoDelete( true );
    globalAdvice = KCookieDunno;    
    configChanged = false;
    cookiesChanged = false;
}

//
// Destructs the cookie jar
//
// Poor little cookies, they will all be eaten by the cookie monster!
//
KCookieJar::~KCookieJar()
{
    // Not much to do here
}

//
// Looks for cookies in the cookie jar which are appropriate for _url.
// Returned is a string containing all appropriate cookies in a format 
// which can be added to a HTTP-header without any additional processing.
//
QString KCookieJar::findCookies(const char *_url)
{
    QString cookieStr("");
    QString domain;
    QString fqdn;
    QString path;
    KCookiePtr cookie;
    int protVersion = 1;
    int cookieCount = 0;
    
    if (!extractDomain(_url, fqdn, domain, path))
    {
        return cookieStr;
    }

    KCookieList *cookieList = cookieDomains[domain.data()];
    
    if (!cookieList)
	return cookieStr; // No cookies for this domain    

    for ( cookie=cookieList->first(); cookie != 0; cookie=cookieList->next() )
    {
        if (!cookie->domain.isEmpty())
        {
            // Cookie has a domain set
            if (domain != cookie->domain)
                continue; // Domain of cookie does not match with host of URL
	}        
	else
        {
            // Cookie has no domain set
            if (fqdn != cookie->host)
                continue; // Host of cookie does not match with host of URL
        }

        if (!cookie->path.isEmpty())
        {
            // Cookie has a path set
            if (path.find(cookie->path.data()) != 0)
                continue; // Path of URL does not start with cookie-path
	}
	// Use first cookie to determine protocol version
	if (cookieCount == 0)
	{
	   protVersion = cookie->protocolVersion;
	}
	if (protVersion == 0)
	{
	    if (cookieCount == 0)
	        cookieStr += "Cookie: ";
	    else
	        cookieStr += "; ";
            cookieStr += cookie->getCookieStr();
	}
	else 
	{
            cookieStr += "Cookie: ";
            cookieStr += cookie->getCookieStr();
            cookieStr += "\r\n";
        } 
        cookieCount++;
    }                    

    if ((protVersion == 0) && (cookieCount > 0))
        cookieStr += "\r\n";
        
    return cookieStr;
}

//
// This function parses a string like 'my_name="my_value";' and returns
// 'my_name' in Name and 'my_value' in Value.
//
// A pointer to the end of the parsed part is returned.
// This pointer points either to:
// '\0' - The end of the string has reached.
// ';'  - Another my_name="my_value" pair follows
// ','  - Another cookie follows
// '\n' - Another header follows
static const char *parseNameValue(const char *header, 
                                  QString &Name, 
                                  QString &Value)
{
    const char* s = header;

    // Skip any whitespace
    for(; (*s == ' ') || (*s == '\t'); s++)
    {           
        if ((*s=='\0') || (*s==';') || (*s=='\n'))
        {
            // End of header
            Name = "";
            Value = "";
            return (s);
        }
    }

    header = s;

    // Parse 'my_name' part
    for(; (*s != '=') && (*s != ' ') && (*s != '\t'); s++)
    {           
        if ((*s=='\0') || (*s==';') || (*s=='\n'))
        {
            // End of Name
            Value = "";
            Name = header;
            Name.truncate( s - header );
            return (s);
        }
    }
    
    Name = header;
    Name.truncate( s - header );

    // Skip any whitespace
    for(; (*s != '='); s++)
    {           
        if ((*s=='\0') || (*s==';') || (*s=='\n'))
        {
            // End of Name
            Value = "";
            return (s);
        }
    }
    
    // *s == '='
    s++;

    // Skip any whitespace
    for(; (*s == ' ') || (*s == '\t'); s++)
    {           
        if ((*s=='\0') || (*s==';') || (*s=='\n'))
        {
            // End of Name
            Value = "";
            return (s);
        }
    }
    
    if (*s == '\"')
    {
        // Parse '"my_value"' part (quoted value)
        s++;  // skip "
        header = s;
        for(;(*s != '\"');s++)
        {
            if ((*s=='\0') || (*s=='\n'))
            {
                // End of Name
                Value = header;
                Value.truncate(s - header);
                return (s);
            }
        }
        Value = header;
        Value.truncate( s - header );

        // *s == '\"';
        s++;
        // Skip any remaining garbage
        for(;; s++)
        {           
            if ((*s=='\0') || (*s==';') || (*s=='\n'))
                break;
        }
    }
    else
    {
        // Parse 'my_value' part (unquoted value)
        header = s;
        while ((*s != '\0') && (*s != ';') && (*s != '\n'))
            s++;
        // End of Name
        Value = header;
        Value.truncate( s - header );
    }
    return (s);

}    

static void stripDomain(const char *_fqdn, QString &_domain)
{
    _domain = _fqdn;
    // Strip trailing dots 
    while((_domain.length() > 0) && (_domain[_domain.length()-1] == '.'))
        _domain.remove(_domain.length()-1, 1);

    int dot_pos = _domain.find('.');

    if (dot_pos != -1)
    {
        // Domain contains a '.': remove the hostname from the domain
        _domain.remove(0, dot_pos);
        
        if (_domain.find('.', 1) == -1)
        {
           // Domain part should contain at least another '.'
           // Use the host part instead, but add a '.' in front
           _domain = ".";
           _domain += _fqdn;
        }
    }

}

static const char haystack[37]="janfebmaraprmayjunjulaugsepoctnovdec";

static time_t parseExpire(const char *expireDate)
{
     // This parse an expire date in the form:
     //     Wednesday, 09-Nov-99 23:12:40 GMT
     // or
     //     Sat, 01-Jan-2000 08:00:00 GMT
     // 
     // We always assume GMT, the weekday is ignored
     //
     time_t result = 0;
     char *newPosStr;
     int day;
     char monthStr[4];
     int month;
     int year;
     int hour;
     int minute;
     int second;
     struct tm tm_s;
     
     while(*expireDate && (*expireDate != ' '))
     	expireDate++;
     
     if (!*expireDate)
     	return result;  // Invalid expire date
     	
     // ' 09-Nov-99 23:12:40 GMT'
     day = strtol(expireDate, &newPosStr, 10);
     expireDate = newPosStr;

     if ((day < 1) || (day > 31))
     	return result; // Invalid expire date;
     if (!*expireDate)
     	return result;  // Invalid expire date

     if (*expireDate++ != '-')
     	return result;  // Invalid expire date

     for(int i=0; i < 3;i++)
     {
         if (!*expireDate || (*expireDate == '-'))
              return result;  // Invalid expire date
         monthStr[i] = tolower(*expireDate++);
     }
     monthStr[3] = '\0';
     
     newPosStr = (char*)strstr(haystack, monthStr);

     if (!newPosStr)
     	return result;  // Invalid expire date

     month = (newPosStr-haystack)/3; // Jan=00, Feb=01, Mar=02, ..

     if ((month < 0) || (month > 11))
     	return result;  // Invalid expire date
       
     while(*expireDate && (*expireDate != '-'))
     	expireDate++;
     
     if (!*expireDate)
     	return result;  // Invalid expire date
     	
     // '-99 23:12:40 GMT'
     if (*expireDate++ != '-')
     	return result;  // Invalid expire date

     // '99 23:12:40 GMT'
     year = strtol(expireDate, &newPosStr, 10);
     expireDate = newPosStr;

     // Y2K: Solve 2 digit years 
     if ((year >= 0) && (year < 50)) 
         year += 2000;  

     if ((year >= 50) && (year < 100)) 
         year += 1900;  // Y2K

     if ((year < 1900) || (year > 2500))
     	return result; // Invalid expire date
         
     if (!*expireDate)
     	return result;  // Invalid expire date

     // ' 23:12:40 GMT'
     if (*expireDate++ != ' ')
     	return result;  // Invalid expire date

     hour = strtol(expireDate, &newPosStr, 10);
     expireDate = newPosStr;

     if ((hour < 0) || (hour > 23))
     	return result; // Invalid expire date
         
     if (!*expireDate)
     	return result;  // Invalid expire date
          	
     // ':12:40 GMT'
     if (*expireDate++ != ':')
     	return result;  // Invalid expire date

     minute = strtol(expireDate, &newPosStr, 10);
     expireDate = newPosStr;

     if ((minute < 0) || (minute > 59))
     	return result; // Invalid expire date
         
     if (!*expireDate)
     	return result;  // Invalid expire date

     // ':40 GMT'
     if (*expireDate++ != ':')
     	return result;  // Invalid expire date

     second = strtol(expireDate, &newPosStr, 10);

     if ((second < 0) || (second > 59))
     	return result; // Invalid expire date
      
     tm_s.tm_sec = second;
     tm_s.tm_min = minute;
     tm_s.tm_hour = hour;
     tm_s.tm_mday = day;
     tm_s.tm_mon = month; 
     tm_s.tm_year = year-1900;
     tm_s.tm_isdst = -1;

#ifndef BSD
     result = mktime( &tm_s)-timezone; // timezone = seconds _west_ of UTC
#else
     result = mktime (&tm_s);
     struct tm *tzone = localtime(&result);
     result += (tzone->tm_gmtoff); // tm_gmtoff = seconds _east_ of UTC
#endif

////////////////
// Debug stuff
//
//   printf("time = %ld sec since epoch, ctime=%s\n", result, ctime(&result));
//
//   time_t now = time(0);
//   printf("now = %ld (expire is in %s)\n", now, result > now ? "future" : "past"); 
////////////////

     return result;  // Invalid expire date
} 
    
bool KCookieJar::extractDomain(const char *_url,
                               QString &_fqdn,
                               QString &_domain,
                               QString &_path)
{
    KURL kurl(_url);    
    
    if ( strncmp(kurl.protocol(), "http", 4) != 0 )
    	return false; // We only do HTTP cookies :)
    
    _fqdn = kurl.host();
    _fqdn = _fqdn.lower();
    stripDomain(_fqdn, _domain);
    _path = kurl.path();
    _path = _path.lower();
            
    return true;
}
                                                   
//
// This function parses cookie_headers and returns a linked list of
// KCookie objects for all cookies found in cookie_headers.
// If no cookies could be found 0 is returned.
//
// cookie_headers should be a concatenation of all lines of a HTTP-header
// which start with "Set-Cookie". The lines should be separated by '\n's.
// 
KCookiePtr KCookieJar::makeCookies(const char *_url, 
                                   const char *cookie_headers)
{
    KCookiePtr cookieChain = 0;
    KCookiePtr lastCookie = 0;
    const char *cookieStr = cookie_headers;
    QString Name;
    QString Value;
    QString fqdn;
    QString domain;
    QString path;
    
    if (!extractDomain(_url, fqdn, domain, path))
    {
        // Error parsing _url
        return 0;
    }
    
    //  The hard stuff :)
    for(;;)
    {
        // check for "Set-Cookie"
        if (strncasecmp(cookieStr, "Set-Cookie: ", 12) == 0)
        {
            cookieStr = parseNameValue(cookieStr+12, Name, Value);

	    if (Name.isEmpty())
	        continue;

            // Host = FQDN
            // Default domain = ""
            // Default path = ""
            lastCookie = new KCookie(fqdn, "", "", 
                                     Name.data(), Value.data() );

            // Insert cookie in chain
            lastCookie->nextCookie = cookieChain;
            cookieChain = lastCookie;
        }
        else if (lastCookie && (strncasecmp(cookieStr, "Set-Cookie2: ", 13) == 0))
        {
            // What the fuck is this? 
            // Does anyone invent his own headers these days? 
            // Read the fucking RFC guys! This header is not there!
            cookieStr +=12;
            // Continue with lastCookie
        }
        else
        {
            // This is not the start of a cookie header, skip till next line.
            while (*cookieStr && *cookieStr != '\n')
                cookieStr++;
            
            if (*cookieStr == '\n')
            	cookieStr++;
             
            if (!*cookieStr)
                break; // End of cookie_headers
            else
                continue; // end of this header, continue with next.
        }
        
        while ((*cookieStr == ';') || (*cookieStr == ' '))
        {
            cookieStr++;

            // Name-Value pair follows
            cookieStr = parseNameValue(cookieStr, Name, Value);
                
            Name = Name.lower();

            if (Name == "domain")
            {
                lastCookie->domain = Value.lower();
            }
            else if (Name == "max-age")
            {
                int max_age = Value.toInt();
                if (max_age == 0)
                    lastCookie->expireDate = 1;
                else
                    lastCookie->expireDate = time(0)+max_age;
            }
            else if (Name == "expires")
            {
                // Parse brain-dead netscape cookie-format
                lastCookie->expireDate = parseExpire(Value.data());
            }
            else if (Name == "path")
            {
                lastCookie->path = Value;
            }
            else if (Name == "version")
            {
                lastCookie->protocolVersion = Value.toInt();
            }
        }
        if (*cookieStr == '\0')
            break; // End of header

        // Skip ';' or '\n'
        cookieStr++;
    }
    
    return cookieChain;
}

//
// This function hands a KCookie object over to the cookie jar.
//
// On return cookiePtr is set to 0.
//
void KCookieJar::addCookie(KCookiePtr &cookiePtr)
{
    QString domain;
    stripDomain( cookiePtr->host, domain);
    KCookieList *cookieList = cookieDomains[domain.data()];
    
    if (!cookieList)
    {
        // Make a cookie list for domain 

        // Make a new cookie list
        cookieList = new KCookieList();
        cookieList->setAdvice( KCookieDunno );
            
        cookieDomains.insert( domain.data(), cookieList);

        // Update the list of domains
        domainList.inSort( domain.data());
    }

    // Look for matching existing cookies
    // They are removed
    for ( KCookiePtr cookie=cookieList->first(); cookie != 0; )
    {
        if ((cookie->name == cookiePtr->name) && 
            (cookie->domain == cookiePtr->domain) &&
            (cookie->path == cookiePtr->path))
        {
            KCookiePtr old_cookie = cookie;
            cookie = cookieList->next(); 
            cookieList->removeRef( old_cookie );
            cookiesChanged = true;
        }
        else
        {
            cookie = cookieList->next();
        }
    }


    // Add the cookie to the cookie list
    // The cookie list is sorted 'longest path first'
    if (!cookiePtr->isExpired(time(0)))
    {
        cookieList->inSort( cookiePtr );
        cookiesChanged = true;
    }
    else
    {
        delete cookiePtr;
    }
    cookiePtr = 0;
}

//
// This function advices whether a single KCookie object should 
// be added to the cookie jar.
//
KCookieAdvice KCookieJar::cookieAdvice(KCookiePtr cookiePtr)
{
    QString domain;
    stripDomain( cookiePtr->host, domain);
    // First check if the domain matches the host
    if (!cookiePtr->domain.isEmpty() &&
        (cookiePtr->domain != domain) && 
        (cookiePtr->domain != cookiePtr->host))
    {
        warning("WARNING: Host %s tries to set cookie for domain %s",
              cookiePtr->host.data(), cookiePtr->domain.data());
        return KCookieReject;
    }

    if ((cookiePtr->name.find('\"') != -1) ||
        (cookiePtr->value.find('\"') != -1))
    {
        warning("WARNING: Host %s tries to set a suspicious cookie for domain %s",
              cookiePtr->host.data(), cookiePtr->domain.data());
        return KCookieReject;
    }

    KCookieList *cookieList = cookieDomains[domain.data()];
    KCookieAdvice advice;
    
    if (cookieList)
    {
        advice = cookieList->getAdvice();
    }
    else
    {
        advice = globalAdvice;
    }

    return advice;    
}

//
// This function gets the advice for all cookies originating from
// _domain.
//
KCookieAdvice KCookieJar::getDomainAdvice(const char *_domain)
{
    KCookieList *cookieList = cookieDomains[_domain];
    KCookieAdvice advice;
    
    if (cookieList)
    {
        advice = cookieList->getAdvice();
    }
    else
    {
        advice = KCookieDunno;
    }

    return advice;    
}
                                                
//
// This function sets the advice for all cookies originating from 
// _domain.    
//
void KCookieJar::setDomainAdvice(const char *_domain, KCookieAdvice _advice)
{
    QString domain(_domain);
    KCookieList *cookieList = cookieDomains[domain.data()];
    
    if (cookieList)
    {
        configChanged = (cookieList->getAdvice() != _advice);
        // domain is already known
        cookieList->setAdvice( _advice);

        if ((cookieList->isEmpty()) && 
            (_advice == KCookieDunno))
        {
            // This deletes cookieList!
            cookieDomains.remove(domain.data());    
            
            domainList.remove(domain.data());            
        }
    }
    else
    {
        // domain is not yet known
        if (_advice != KCookieDunno)
        {
            // We should create a domain entry
            configChanged = true;
            
            // Make a new cookie list
            cookieList = new KCookieList();
            cookieList->setAdvice( _advice);
            
            cookieDomains.insert( domain.data(), cookieList);

            // Update the list of domains
            domainList.inSort( domain.data());
        }
    }
        
}

//
// This function sets the advice for all cookies originating from 
// the same domain as _cookie    
//
void KCookieJar::setDomainAdvice(KCookiePtr _cookie, KCookieAdvice _advice)
{
    QString domain;
    stripDomain( _cookie->host, domain);
    setDomainAdvice(domain.data(), _advice);
}

//
// This function sets the global advice for cookies 
//
void KCookieJar::setGlobalAdvice(KCookieAdvice _advice)
{
    configChanged = (globalAdvice != _advice);
    globalAdvice = _advice;
}

//
// Get a list of all domains known to the cookie jar.
//
const QStrList *KCookieJar::getDomainList()
{
    return &domainList;
}
    
//
// Get a list of all cookies in the cookie jar originating from _domain.
//
const KCookieList *KCookieJar::getCookieList(const char *_domain)
{
    return cookieDomains[_domain];
}

//
// Eat a cookie out of the jar. 
// cookiePtr should be one of the cookies returned by getCookieList()
//
void KCookieJar::eatCookie(KCookiePtr cookiePtr)
{
    QString domain;
    stripDomain( cookiePtr->host, domain);
    KCookieList *cookieList = cookieDomains[domain.data()];
    
    if (cookieList)
    {
        // This deletes cookiePtr!
        cookieList->removeRef( cookiePtr );

        if ((cookieList->isEmpty()) && 
            (cookieList->getAdvice() == KCookieDunno))
        {
            // This deletes cookieList!
            cookieDomains.remove(domain.data());    
            
            domainList.remove(domain.data());            
        }
    }
}    

//
// Saves all cookies to the file '_filename'.
// On succes 'true' is returned.
// On failure 'false' is returned.
bool KCookieJar::saveCookies(const char *_filename)
{
    QString backupFile(_filename);
    backupFile += ".bak";
    // We try to make a backup first. 
    (void) rename(_filename, backupFile.data());
    // We don't care if it fails
    
    // We try to exclusively create our cookie file.
    // If an existing cookie-file (or link to /etc/passwd) could not 
    // be renamed into a backup file the rename command above then 
    // this call will fail.
    int fd = open(_filename, O_WRONLY| O_CREAT | O_EXCL, S_IRUSR | S_IWUSR);

    if (fd == -1)
    	return false;

    FILE *fStream = fdopen( fd, "w");
    if (fStream == 0)
    {
        close(fd);
        return false;
    }
    
    time_t curTime = time(0);
    
    fprintf(fStream, "# KDE Cookie File\n#\n");
    
    fprintf(fStream, "%-20s %-20s %-12s %-9s %-4s %-10s %s\n", 
    	"# Host", "Domain", "Path", "Exp.date", "Prot", "Name", "Value");

    for(const char *domain = domainList.first();
    	domain != 0;
    	domain = domainList.next())
    {
        bool domainPrinted = false;

	KCookieList *cookieList = cookieDomains[domain];
        KCookiePtr cookie=cookieList->first();

	for (; cookie != 0;)
        {
            if (cookie->isExpired(curTime))
	    {
	    	// Delete expired cookies
                KCookiePtr old_cookie = cookie;
                cookie = cookieList->next(); 
                cookieList->removeRef( old_cookie );
	    }
	    else if (cookie->expireDate != 0)
            {
                if (!domainPrinted)
                {
                    domainPrinted = true;
                    fprintf(fStream, "[%s]\n", domain);
                }
                // Store persistent cookies
                QString path("\"");
                path += cookie->path.data();
                path += "\"";  
                QString domain("\"");
                domain += cookie->domain.data();
                domain += "\"";  
                fprintf(fStream, "%-20s %-20s %-12s %9lu   %2d %-10s \"%s\"\n", 
		    cookie->host.data(), domain.data(), path.data(), 
		    (unsigned long) cookie->expireDate, 
		    cookie->protocolVersion, 
		    cookie->name.data(), cookie->value.data());
		cookie = cookieList->next();
	    }
	    else
	    {
	    	// Skip session-only cookies 
		cookie = cookieList->next();
	    }
	}
    }    
    
    if (fclose( fStream) == -1)
    {
    	// Error
        return false;
    }
    // Succes!
    return true;
}

typedef char *charPtr;

static const char *parseField(charPtr &buffer)
{
    char *result;
    if (*buffer == '\"')
    {
    	// Find terminating "
    	buffer++;
    	result = buffer;
        while((*buffer != '\"') && (*buffer))
    	    buffer++;
    }
    else 
    {
        // Find first white space
        result = buffer;
        while((*buffer != ' ') && (*buffer != '\t') && (*buffer != '\n') && (*buffer))
    	    buffer++;
    }	    	
    if (!*buffer)
    	return result; // 
    *buffer++ = '\0';
    
    // Skip white-space
    while((*buffer == ' ') || (*buffer == '\t') || (*buffer == '\n'))
    	buffer++;

    return result;
}


//
// Reloads all cookies from the file '_filename'.
// On succes 'true' is returned.
// On failure 'false' is returned.
bool KCookieJar::loadCookies(const char *_filename)
{
    FILE *fStream = fopen( _filename, "r");
    if (fStream == 0)
    {
        return false;
    }
    
    time_t curTime = time(0);
    
    char *buffer = new char[READ_BUFFER_SIZE];

    bool err = false;    
    err = (fgets(buffer, READ_BUFFER_SIZE, fStream) == 0);
    
    err = err || (strcmp(buffer, "# KDE Cookie File\n") != 0);
    
    if (!err)
    {
    	while(fgets(buffer, READ_BUFFER_SIZE, fStream) != 0)
    	{
            char *line = buffer;
    	    // Skip lines which begin with '#' or '['
    	    if ((line[0] == '#') || (line[0] == '['))
    	    	continue;
	    
	    const char *host( parseField(line) );
	    const char *domain( parseField(line) );
	    const char *path( parseField(line) );
	    const char *expStr( parseField(line) );
	    if (!expStr) continue;
	    int expDate  = (time_t) strtoul(expStr, 0, 10);
	    const char *verStr( parseField(line) );
	    if (!verStr) continue;
	    int protVer  = (time_t) strtoul(verStr, 0, 10);
	    const char *name( parseField(line) );
	    const char *value( parseField(line) );

	    // Parse error
	    if (!value) continue;

	    // Expired or parse error
	    if ((expDate == 0) || (expDate < curTime))
	    	continue;
	    	
            // Domain must be either hostname or domain of hostname
	    if ( strlen(domain) && (strcmp(host,domain) != 0))
	    {
                QString checkDomain;
                stripDomain( host, checkDomain);
                if (checkDomain != domain)
                    continue;
            }

	    KCookie *cookie = new KCookie(host, domain, path, 
	    	name, value, expDate, protVer);
	    addCookie(cookie);	                                      
    	}
    }
    delete [] buffer;

    fclose( fStream);
    return err;
}

static const char *adviceToStr(KCookieAdvice _advice)
{
    switch( _advice )
    {
    case KCookieAccept: return "Accept";
    case KCookieReject: return "Reject";
    case KCookieAsk: return "Ask";
    default: return "Dunno";
    }
}

static KCookieAdvice strToAdvice(const char *_str)
{
    if (!_str)
        return KCookieDunno;
        
    if (strcasecmp(_str, "Accept") == 0)
	return KCookieAccept;
    else if (strcasecmp(_str, "Reject") == 0)
	return KCookieReject;
    else if (strcasecmp(_str, "Ask") == 0)
	return KCookieAsk;

    return KCookieDunno;	
}


//
// Save the cookie configuration
//

void KCookieJar::saveConfig(KConfig *_config)
{
    if (!configChanged)
        return;

    QStrList domainSettings;
    _config->setGroup("Browser Settings/HTTP");
    _config->writeEntry("CookieGlobalAdvice", adviceToStr( globalAdvice));
    
    for ( char *domain=domainList.first(); 
    	  domain != 0; 
    	  domain = domainList.next())
    {
         KCookieAdvice advice = getDomainAdvice( domain);
         if (advice != KCookieDunno)
         {
             QString value(domain);
             value += ':';
             value += adviceToStr(advice);
             domainSettings.append(value.data());
         }
    }
    _config->writeEntry("CookieDomainAdvice", domainSettings);
    _config->sync();
}
 
 
//
// Load the cookie configuration
//

void KCookieJar::loadConfig(KConfig *_config)
{
    QString value;
    QStrList domainSettings;
    // Reset current domain settings first
    
    for ( char *domain=domainList.first(); 
    	  domain != 0; 
    	  domain = domainList.next())
    {
         setDomainAdvice( domain, KCookieDunno);
    }

    _config->setGroup("Browser Settings/HTTP");
    value = _config->readEntry("CookieGlobalAdvice", "Ask");
    globalAdvice = strToAdvice(value.data());

    (void) _config->readListEntry("CookieDomainAdvice", domainSettings);
    for (char *value = domainSettings.first();
         value != 0;
         value = domainSettings.next())
    {
    	QString strVal(value);
        int sepPos = strVal.find(':');
        if (sepPos <= 0)
             continue;
        QString domain(strVal.left(sepPos));
        KCookieAdvice advice = strToAdvice(strVal.data()+sepPos + 1);
        setDomainAdvice( domain, advice);
    }
}
                                    
