/*
 * $Id$
 *
 * $Log$
 * Revision 1.3.2.1  1997/01/10 19:48:32  alex
 * public release 0.1
 *
 * Revision 1.3  1997/01/10 19:44:33  alex
 * *** empty log message ***
 *
 * Revision 1.2.4.1  1997/01/10 16:46:33  alex
 * rel 0.1a, not public
 *
 * Revision 1.2  1997/01/10 13:05:52  alex
 * *** empty log message ***
 *
 * Revision 1.1.1.1  1997/01/10 13:05:21  alex
 * imported
 *
 */

// kmsgbox.h

#ifndef _KMSGBOX_H_
#define _KMSGBOX_H_

#include <qdialog.h>
#include <qpushbt.h>
#include <qlabel.h>
#include <qframe.h>

#include <kpixmap.h>


/// KMsgBox
/** A message box API for the KDE project. KMsgBox provides a Windows - lookalike message- or
 error box with icons and up to 4 configurable buttons.
 */

class KMsgBox : public QDialog
{
    Q_OBJECT;
public:

    enum {INFORMATION = 1, EXCLAMATION = 2, STOP = 4, QUESTION = 8};
    enum {DB_FIRST = 16, DB_SECOND = 32, DB_THIRD = 64, DB_FOURTH = 128};
    
    /// Constructor
    /** The generic constructor for a KMsgBox Widget. All parameters have a default value of 0.
     @Doc:
     \begin{itemize}
     \item { \tt The parent widget }
     \item { \tt A caption (title) for the message box. }
     \item { \tt a message string. The default alignment is centered. The string may contain
     newline characters.}
     \item { \tt Flags: This parameter is responsible for several behaviour options of the
     message box. See below for valid constans for this parameter.}
     \item { \tt Up to 4 button strings (b1text to b4text). You have to specify at least b1text.
     Unspecified buttons will not appear in the message box, therefore you can control the
     number of buttons in the box.}
     \end{itemize}
     */
    KMsgBox(QWidget *parent = 0, const char *caption = 0, const char *message = 0, int flags = INFORMATION,
            const char *b1text = 0, const char *b2text = 0, const char *b3text = 0, const char *b4text = 0);

    /// yesNo
    /** A static member function. Can be used to create simple message boxes with a
     single function call. This box has 2 buttons which default to "Yes" and "No". The icon
     shows a question mark. The appearance can be manipulated with the {\bf type} parameter.
     */
    
    static int yesNo(QWidget *parent = 0, const char *caption = 0, const char *message = 0, int type = 0,
                     const char *yes = "Yes", const char *no = "No");

    /// yesNoCancel
    /** A static member function for creating a three-way message box. The box has three
     buttons (defaulting to "Yes", "No" and "Cancel") and a question-mark icon. The {\bf type}
     Parameter can be used to change the default behaviour.
     */
    
    static int yesNoCancel(QWidget *parent = 0, const char *caption = 0, const char *message = 0, int type = 0,
                           const char *yes = "Yes", const char *no = "No", const char *cancel = "Cancel");

    /// message
    /** A static member function. Creates a simple message box with only one button ("Ok")
     and a information-icon. The icon can be changed by using another value for the {\bf type}
     parameter.
     */
    
    static int message(QWidget *parent = 0, const char *caption = 0, const char *message = 0, int type = 0,
                       const char *btext = "Ok");


private:
    enum {B_SPACING = 10, B_WIDTH = 80};
    QLabel      *msg, *picture;
    QPushButton *b1, *b2, *b3, *b4;
    QFrame      *f1;
    int nr_buttons;
    int         w, h, h1, text_offset;
    void        calcOptimalSize();
    void        resizeEvent(QResizeEvent *);

    void        initMe(const char *caption, const char *message, const char *b1text,
                       const char *b2text, const char *b3text, const char *b4text,
                       const KPixmap & icon = 0);

public slots:
    void        b1Pressed();
    void        b2Pressed();
    void        b3Pressed();
    void        b4Pressed();
};

#endif

