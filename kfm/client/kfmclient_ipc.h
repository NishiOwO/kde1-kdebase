// This file has been created by ipcc.pl.
// (c) Torben Weis
//     weis@stud.uni-frankfurt.de

#ifndef KfmIpc_h
#define KfmIpc_h

#include <ctype.h>
#include <ksock.h>
#include <qobject.h>
#include "ipc.h"

class KfmIpc : public QObject
{
    Q_OBJECT
public:
    KfmIpc( int _port );
    ~KfmIpc();

    bool isConnected();

public slots:
	void refreshDesktop();
public slots:
	void refreshDirectory(const char* _url);
public slots:
	void openURL(const char* _url);
public slots:
	void openProperties(const char* _url);
public slots:
	void exec(const char* _url, const char* _binding);
public slots:
	void copy(const char* _src, const char* _dest);
public slots:
	void move(const char* _src, const char* _dest);
public slots:
	void moveClient(const char* _src, const char* _dest);
public slots:
	void ask(int _x, int _y, const char* _src, const char* _dest);
signals:
	void finished();
private:
	void parse_finished( char *_data, int _len );
public slots:
    void readEvent( KSocket * );
    void closeEvent( KSocket * );
private:
    void parse( char *_data, int _len );

    int port;
    KSocket *sock;
    bool connected;
    char headerBuffer[11];
    int cHeader;
    bool bHeader;
    char *pBody;
    int cBody;
    int bodyLen;
};

#endif
